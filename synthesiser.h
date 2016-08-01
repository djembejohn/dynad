// Copyright 2016 John Bryden

// This file is part of Dynad.

//    Dynad is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    Dynad is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.

//    You should have received a copy of the GNU Affero General Public License
//    along with Dynad.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#include <vector>
#include "controller.h"
#include "boost/thread/mutex.hpp"
#include "envelope.h"
#include "oscillator.h"
#include "bcr2000Driver.h"
#include "FreeVerb.h"
#include "Echo.h"
#include "Chorus.h"

using namespace std;
using namespace jab;

extern double midiNoteToFrequency (int note);
extern double stringToFrequency (string note);
extern int stringToMidiNote (string note);

using namespace std;

extern ofstream outlog;

class Partial
{
public:
  
  Oscillator oscillator;

  list<REnvelope> envelopes;
  RController masterVolume;
  RController pan;

  Partial (RController _masterVolume, RController _pan, RController level, RController noteFrequency, RController frequencyMultiplier, RController frequencyNoise = 0, RController distortion = 0, RController squareshape = 0, RController sawshape = 0, RController chorus = 0)
    : oscillator (level, frequencyMultiplier, noteFrequency, frequencyNoise, distortion, squareshape, sawshape, chorus),
    masterVolume(_masterVolume), pan(_pan)
    {}
    
  bool attachController(RController con){
    return oscillator.attachController(con);
  }
  
  void playToThreadSafeBuffer(RBufferWithMutex threadSafeBuffer) {
    //    cout << "partial " << threadSafeBuffer->timeStart 
    //	 << " " << threadSafeBuffer->numberOfFrames
    //	 << " " << threadSafeBuffer->sampleRate << endl;

    const size_t subBufferIncrement=256;

    double internalTimeCounter = threadSafeBuffer->timeStart;
    double sampleRate = threadSafeBuffer->sampleRate;
    int numChannels = threadSafeBuffer->numberOfChannels;

    vector<StkFloat> subBuffer (subBufferIncrement*numChannels,0.0);
    size_t bufferNumFrames = threadSafeBuffer->numberOfFrames;

    for (size_t i = 0; i<bufferNumFrames; i += subBufferIncrement) {
      size_t thisBufferNumFrames = subBufferIncrement;
      if (i+subBufferIncrement > bufferNumFrames) {
	thisBufferNumFrames = bufferNumFrames-i;
      }
      playToBuffer(subBuffer,sampleRate,thisBufferNumFrames,numChannels,internalTimeCounter);
      threadSafeBuffer->lock();
      safeVector<StkFloat> & outbuf = threadSafeBuffer->getOutBufRef();
      for (size_t j = 0; j<thisBufferNumFrames*numChannels; j++) {
	StkFloat value = outbuf[i*numChannels+j]+subBuffer[j];
	outbuf[i*numChannels+j] = value;
	subBuffer[j] = 0;
      }
      internalTimeCounter += (double)thisBufferNumFrames/sampleRate;
      //      cout << internalTimeCounter << endl;
      threadSafeBuffer->unlock();
    }
    //    timeCounter += (double)bufferNumFrames/sampleRate;

  }

  // The envelopes stack on top of one another, multiplying together
  // Envelopes should return levels between 0 and 1
  void addEnvelope (REnvelope env)
  {
    envelopes.push_back (env);
  }

  // The amplitude oscillators stack on top of one another, adding
  // together
  void setAmplitudeOscillator (shared_ptr<Oscillator> osc)
  {
    oscillator.amplitudeOscillator=osc;
  }

  void setFrequencyOscillator (shared_ptr<Oscillator> osc)
  {
    oscillator.frequencyOscillator=osc;
  }


  
 private:

  void playToBuffer (vector<StkFloat> & buffer, double sampleRate, unsigned int numFrames, int numChannels, double tStart) 
  {
    for (unsigned int i = 0; i<numFrames; i++) {
      double timePoint = tStart+(double)i/sampleRate;
      double level = 1.0;
      //      for (auto & osc:amp_oscillators) {
      //	level = level + osc.getValue(timePoint);
      //      }

      for (auto & env:envelopes) 
	level = level * env->getBufferedLevel(timePoint);
      StkFloat value = 0.0;
      if (level > 0.0) 
	value = oscillator.getValue(timePoint);

      // must implement pan ;)
      if (value*level != 0.0) {
	
	if (numChannels>1) {
	  double panLevel = pan->getOutputLevel();
	  
	  double lval = value*level*panLevel*2.0;
	  double rval = value*level*(1.0-panLevel)*2.0;
	  buffer[i*numChannels] = lval;
	  buffer[i*numChannels+1] = rval;
	}
	else
	  buffer[i*numChannels] = value*level;
      }


    }
  }

  //  double timeCounter;

};

typedef shared_ptr<Partial> RPartial;



class PartialsList : public boost::mutex
{
 public:
  list<RPartial> partials;
};

typedef shared_ptr<PartialsList> RPartialsList;

class ThreadedPartialPlayer
{
 public:
  ThreadedPartialPlayer (RPartialsList _partialList, RBufferWithMutex _threadSafeBuffer)
    : partialList (_partialList), 
    threadSafeBuffer(_threadSafeBuffer)
    {}
  
  void threadedFunction() {
    bool finished = false;
    while (!finished) {
      RPartial thisPartial = 0;
      partialList->lock();

      //      cout << partialList->partials.size() << endl;
      list<RPartial>::iterator pi = partialList->partials.begin();
      if (pi != partialList->partials.end()) {
	thisPartial = *pi;
	partialList->partials.pop_front();
      }
      partialList->unlock();
      if (thisPartial) {
	thisPartial->playToThreadSafeBuffer(threadSafeBuffer);
      }
      else 
	finished = true;
    }
  }
    
  RPartialsList partialList;
  RBufferWithMutex threadSafeBuffer;

};



class Phone
{
 public:  
  RControllerSet controlSet;
  RController masterVolume;

  list<RPartial> partials;
  REnvelope phoneEnvelope;
  RNoteControlledFrequency masterNote;

  double baseFrequency = 440.0;
  int numPartials;
  //  double t = 0;

  Phone (RControllerSet _controlSet, RController _masterVolume, int _numPartials) 
    : controlSet(_controlSet), masterVolume (_masterVolume), numPartials (_numPartials)
    {
      RController attack = controlSet->getController ("masterAttack");
      RController decay = controlSet->getController ("masterDecay");
      RController sustain  = controlSet->getController ("masterSustain");
      RController release = controlSet->getController ("masterRelease");
      
      phoneEnvelope = REnvelope(new Envelope (attack,decay,sustain,release));
      KnobController cvar_note (0, 0);
      RNoteControlledFrequency note (new NoteControlledFrequency ("note",440));
      masterNote = note;

      // need to think a bit more about how controllers work for notes
      // on and off etc

      // basically, need to implement a layer of controllers which can
      // be talked to by midi or OSC or anything really

      for (int i = 0; i<numPartials; i++) {
	
	RController freqmult = controlSet->getController("freqMult",i);
	RController level = controlSet->getController("level",i);

	RController pan = controlSet->getController("pan",i);
	RController chorus = controlSet->getController("chorus",i);
	
	RController frequencyNoise = controlSet->getController("frequencyNoise",i);
	RController shape = controlSet->getController("shape",i);
	RController saw = controlSet->getController("saw",i);
	RController distortion = controlSet->getController("distortion",i);

	

	RPartial newPartial = RPartial(new Partial (masterVolume, pan, level, note.GetPointer(), freqmult, frequencyNoise, distortion,shape,saw,chorus));
	newPartial->addEnvelope (phoneEnvelope);

	RController amposclevel = controlSet->getController("amposclevel",i);
	RController amposcfreq  = controlSet->getController("amposcfreq",i);
	RController freqosclevel = controlSet->getController("freqosclevel",i);
	RController freqoscfreq  = controlSet->getController("freqoscfreq",i);

	ROscillator amplitudeOscillator (new Oscillator (amposclevel,amposcfreq));
	ROscillator frequencyOscillator (new Oscillator(freqosclevel,freqoscfreq));
	newPartial->setAmplitudeOscillator(amplitudeOscillator);
	newPartial->setFrequencyOscillator(frequencyOscillator);


	partials.push_back (newPartial);
      }
    }
  
 
  void playToBuffer (RBufferWithMutex buffer) {
    // the first partial is assumed to be the root
    //    RPartial firstPartial = *partials.begin();

    if (!phoneEnvelope->isPlaying())
      return;

    // Set up the envelope
    double tStart = buffer->timeStart;
    double tInc = 1.0/(double)(buffer->sampleRate);
    double tEnd = tStart +tInc*(double)buffer->numberOfFrames;
    phoneEnvelope->prepareBuffer (tStart,tEnd,tInc);

    for (auto partial:partials) {
     partial->playToThreadSafeBuffer (buffer);
    }

#if 0
    ThreadedPartialPlayer threadplayer1(plist,buffer);
    ThreadedPartialPlayer threadplayer2(plist,buffer);
    ThreadedPartialPlayer threadplayer3(plist,buffer);
    ThreadedPartialPlayer threadplayer4(plist,buffer);
    boost::thread thread1 ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer1));
    boost::thread thread2 ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer2));
    boost::thread thread3 ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer3));
    boost::thread thread4 ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer4));
    thread4.join();
    thread3.join();
    thread2.join();
    thread1.join();
    //    thread2.startThread();
    // thread1.waitForThread();
       //    thread2.waitForThread();
    //    thread1.stopThread();
    //    thread2.stopThread();
#elif 0
    ThreadedPartialPlayer threadplayer1(plist,buffer);
    ThreadedPartialPlayer threadplayer2(plist,buffer);
    //    ThreadedPartialPlayer threadplayer3(plist,buffer);
    //    ThreadedPartialPlayer threadplayer4(plist,buffer);

    boost::thread_group group;

    group.create_thread  ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer1));
    group.create_thread  ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer2));
    //    group.create_thread  ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer3));
    //    group.create_thread  ( boost::bind (&ThreadedPartialPlayer::threadedFunction, &threadplayer4));
    group.join_all();

#elif 0    
    ThreadedPartialPlayer threadplayer1(plist,buffer);
    threadplayer1.threadedFunction();
#endif

    //    t += (double)buffer.getNumFrames()/(double)buffer.getSampleRate();

  }
  
};

typedef shared_ptr<Phone> RPhone;

typedef pair<RPhone,RBufferWithMutex> PhoneBufferPair;
 
class PhonesBuffersList : public boost::mutex
{
 public:

  void addPair (RPhone phone,RBufferWithMutex buffer) {
    phoneBuffers.push_back (PhoneBufferPair (phone,buffer));
  }

  PhoneBufferPair popFirst() {
    pair<RPhone,RBufferWithMutex> thisPair;
    lock();
    list<PhoneBufferPair>::iterator pi = phoneBuffers.begin();
    if (pi != phoneBuffers.end()) {
      thisPair = *pi;
      phoneBuffers.pop_front();
    }
    unlock();
    return thisPair;
  }
 private:
  list<PhoneBufferPair > phoneBuffers;
};

typedef shared_ptr<PhonesBuffersList> RPhonesBuffersList;

class ThreadedPhonePlayer
{
 public:
  ThreadedPhonePlayer (RPhonesBuffersList _phoneBufferList)
    : phonesBuffersList (_phoneBufferList)
    {}
  
  void threadedFunction() {
    int counter = 0;
    bool finished = false;
    while (!finished) {
      PhoneBufferPair thispair = phonesBuffersList->popFirst();
      if (thispair.first && thispair.second) {
	thispair.first->playToBuffer(thispair.second);
	counter ++;
      }
      else 
	finished = true;
    }
  }
    
  RPhonesBuffersList phonesBuffersList;

};



class MonoSynthesiser
{
 public:
  RControllerSet controlSet;
  RController masterVolume;

  double t = 0;

  
  RPhone phone;

  MonoSynthesiser () 
    :numNotesOn (0), notesOn (256,0) {
    controlSet = RControllerSet (new ControllerSet(16));

    KnobController cvar_volume (16, 0);
    RController volume (new ControllableLevelLinear ("masterVolume",0.1));
    masterVolume = volume;
    controlSet->addController (volume,16,0);

    phone = RPhone (new Phone (controlSet, volume,16));
    
  }

  void receiveNamedControlMessage (string name,int val) {
    RController controller = controlSet->getController(name);
    if (controller) 
      controller->receiveMidiControlMessage (val);
  }

  void interpretControlMessage (KnobController con, int val) {
    RController controller = controlSet->getController(con);
    if (controller) 
      controller->receiveMidiControlMessage (val);

  }

  void playToBuffer (RBufferWithMutex buffer) {
    phone->playToBuffer(buffer);

    //    t += (double)buffer->numFrames/(double)buffer->sampleRate;


#if 0
    // double internalPhase =firstPartial->oscillator.getInternalPhase();
    //   size_t phaseLag = buffer.getSampleRate()*internalPhase/(2.0*local_pi);
    double phaseLag = 0;
    for (size_t i = 0; i< buffer.getNumFrames(); i++) {
      size_t phasePos = i+phaseLag;
      while (phasePos >= buffer.getNumFrames())
	phasePos -= buffer.getNumFrames();
      if (phasePos >= showBuffer.size())
	cout << phasePos << endl;
      else{ 
	//	cout << internalPhase << " " << phasePos << " " << buffer[i*buffer.getNumChannels()] << endl;
	showBuffer[phasePos] = buffer[i*buffer.getNumChannels()];
      }
    }
#endif

  }

  void noteOn (int noteVal, int velocity) {
    if (noteVal < 0 || noteVal > 255)
      return;
    if (!notesOn[noteVal]) {
      notesOn[noteVal] = true;
      numNotesOn ++;
      phone->phoneEnvelope->noteOn(t, (double)velocity/127.0);
      phone->masterNote->receiveMidiControlMessage(noteVal);
    }
  }

  void noteOff (int noteVal, int velocity) {
    if (noteVal <0 || noteVal > 255)
      return;
    if (notesOn[noteVal]) {
      notesOn[noteVal] = false;
      numNotesOn --;
    }
    if (numNotesOn == 0)
      phone->phoneEnvelope->noteOff(t);
  }

  int numNotesOn;
  vector<bool> notesOn;


};

class FX
{
public:
  FX (RControllerSet controlSet)
    : echo (48000*5),
    chorusModDepth (controlSet->getController("chorusModDepth")),
    chorusModFrequency (controlSet->getController("chorusModFrequency")),
    chorusMix (controlSet->getController("chorusMix")),
    echoDelay (controlSet->getController("echoDelay")),
    echoFeedback (controlSet->getController("echoFeedback")),
    echoMix (controlSet->getController("echoMix")),
    reverbRoomSize (controlSet->getController("reverbRoomSize")),
    reverbDamping (controlSet->getController("reverbDamping")),
    reverbWidth (controlSet->getController("reverbWidth")),
    reverbMix (controlSet->getController("reverbMix"))
      {}

  Chorus chorus;
  Echo echo;
  FreeVerb reverb;

  void updateFXFromControllers () {    
    chorus.setModDepth(chorusModDepth->getLevel());
    chorus.setModFrequency(chorusModFrequency->getLevel());

    echo.setDelay(echoDelay->getLevel());
  
    reverb.setRoomSize(reverbRoomSize->getLevel());
    reverb.setDamping(reverbDamping->getLevel());
    reverb.setWidth(reverbWidth->getLevel());
  }

  RController chorusModDepth;
  RController chorusModFrequency;
  RController chorusMix;
  
  RController echoDelay;
  RController echoFeedback;
  RController echoMix;
  
  RController reverbRoomSize;
  RController reverbDamping;
  RController reverbWidth;
  RController reverbMix;

};


class PolySynthesiser : public boost::mutex
{
 public:
  int numPartials;
  int numPhones;

  RControllerSet controlSet;
  RMorphControllerSet morphSet;

  FX fx;

  ofstream recording;

  vector<RPhone> phones;
  vector<RPhone> notesOn;
  vector<double> timePhoneStarted;

  map<RPhone,int> noteBeingPlayed;

  PolySynthesiser (int numPartials=16, int numPhones=12);

  void save(string filename);
  void load(string filename);
  void loadMorph(string fname1, string fname2);
  void interpretControlMessage (KnobController con, int val);
  void interpretNamedControlMessage (string & controlName, int val);
  bool interpretOSCControlMessage (string & controlName, double val);
  const vector<RPhone> & getPhones();
  void playToBuffer (RBufferWithMutex buffer);
  void noteOn (int noteVal, int velocity, double timePoint);
  void noteOff (int noteVal, int velocity, double timePoint);
  void updateController(); 
  void updateOSCController(); 
};

typedef shared_ptr<PolySynthesiser> RPolySynthesiser;

