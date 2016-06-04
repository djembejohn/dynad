#pragma once
#include <vector>
#include "controller.h"
#include "boost/thread/mutex.hpp"
#include "envelope.h"
#include "oscillator.h"
#include "bcr2000Driver.h"

using namespace std;
using namespace jab;

extern double midiNoteToFrequency (int note);
extern double stringToFrequency (string note);
extern int stringToMidiNote (string note);

using namespace std;


class Partial
{
public:
  
  Oscillator oscillator;

  list<REnvelope> envelopes;
  RController masterVolume;

  Partial (RController _masterVolume, RController level, RController noteFrequency, RController frequencyMultiplier, RController frequencyNoise = 0, RController distortion = 0, RController shape = 0)
    : oscillator (level, frequencyMultiplier, noteFrequency, frequencyNoise, distortion, shape),
    masterVolume(_masterVolume)
    {}
    
  void playToThreadSafeBuffer(RBufferWithMutex threadSafeBuffer) {
    //    cout << "partial " << threadSafeBuffer->timeStart 
    //	 << " " << threadSafeBuffer->numberOfFrames
    //	 << " " << threadSafeBuffer->sampleRate << endl;
    const size_t subBufferIncrement=256;
    double timeCounter = threadSafeBuffer->timeStart;
    double sampleRate = threadSafeBuffer->sampleRate;
    int numChannels = threadSafeBuffer->numberOfChannels;
    vector<float> subBuffer (subBufferIncrement*numChannels,0.0);
    size_t bufferNumFrames = threadSafeBuffer->numberOfFrames;

    for (size_t i = 0; i<bufferNumFrames; i += subBufferIncrement) {
      timeCounter += (double)subBufferIncrement/sampleRate;
      size_t thisBufferNumFrames = subBufferIncrement;
      if (i+subBufferIncrement > bufferNumFrames) {
	thisBufferNumFrames = bufferNumFrames-i;
      }
      playToBuffer(subBuffer,sampleRate,thisBufferNumFrames,numChannels,timeCounter);
      threadSafeBuffer->lock();
      float * outbuf = threadSafeBuffer->getOutBuf();
      for (size_t j = 0; j<thisBufferNumFrames*numChannels; j++) {
	outbuf[i*numChannels+j] += subBuffer[j];
	subBuffer[j] = 0;
      }
      threadSafeBuffer->unlock();
    }

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


#if 0
  void PlayToWave (Wave & wave, double start=0) 
  {
    size_t buffInc = start*wave.sampleRate;
    double maxrel = 0;
    for (auto & env:envelopes) {
      maxrel = max(env.release,maxrel);
    }
    
    for (unsigned int i = 0; i<(noteLength+maxrel)*wave.sampleRate; i++) {
      double timePoint = double(i)/wave.sampleRate;
      double level = 1;
      for (auto & osc:amp_oscillators) {
	level = level + osc.getValue(timePoint);
      }
      for (auto & env:envelopes) {
	level = level * env.getLevel(timePoint,noteLength);
      }
      double value = oscillator.getValue(timePoint);

      if (i+buffInc > wave.buffer.size()) {
	cerr << "Error writing past the end of the wave buffer." << endl;
	break;
      }
      wave.buffer[i+buffInc] += value * level;
    }
  }

#endif

  
 private:

  void playToBuffer (vector<float> & buffer, double sampleRate, unsigned int numFrames, int numChannels, double tStart) 
  {
    for (unsigned int i = 0; i<numFrames; i++) {
      double timePoint = tStart+(double)i/sampleRate;
      double level = masterVolume->getLevel();
      //      for (auto & osc:amp_oscillators) {
      //	level = level + osc.getValue(timePoint);
      //      }
      for (auto & env:envelopes) {
      	level = level * env->getLevel(timePoint);
      }
      double value = oscillator.getValue(timePoint);
      // must implement pan ;)
      for (int j = 0; j<numChannels; j++)
	buffer[i*numChannels+j] += value * level;
    }
  }

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
  REnvelope masterEnvelope;
  RNoteControlledFrequency masterNote;

  double baseFrequency = 440.0;
  //  double t = 0;

  Phone (RControllerSet _controlSet, RController _masterVolume) 
    : controlSet(_controlSet), masterVolume (_masterVolume)
    {
      masterEnvelope = REnvelope(new Envelope (0.6,0.0,1.0,2.0));
      KnobController cvar_note (0, 0);
      RNoteControlledFrequency note (new NoteControlledFrequency (440));
      masterNote = note;

      // need to think a bit more about how controllers work for notes
      // on and off etc

      // basically, need to implement a layer of controllers which can
      // be talked to by midi or OSC or anything really

      for (int i = 0; i< 16; i++) {
	
	RController freqmult = controlSet->getController("freqMult",i);
	RController level = controlSet->getController("level",i);
	RController frequencyNoise = controlSet->getController("frequencyNoise",i);
	RController shape = controlSet->getController("shape",i);
	RController distortion = controlSet->getController("distortion",i);

	RPartial newPartial = RPartial(new Partial (masterVolume, level, note.GetPointer(), freqmult, frequencyNoise, distortion,shape));
	newPartial->addEnvelope (masterEnvelope);

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
    RPartial firstPartial = *partials.begin();

    RPartialsList plist = RPartialsList(new PartialsList());
    for (auto partial:partials) {
      plist->partials.push_back(partial);
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
#elif 1
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

#else    
    ThreadedPartialPlayer threadplayer1(plist,buffer);
    threadplayer1.threadedFunction();
#endif

    //    t += (double)buffer.getNumFrames()/(double)buffer.getSampleRate();

  }
  
};

typedef shared_ptr<Phone> RPhone;

class MonoSynthesiser
{
 public:
  RControllerSet controlSet;
  RController masterVolume;

  double t = 0;

  
  RPhone phone;

  MonoSynthesiser () 
    :numNotesOn (0), notesOn (256,0) {
    controlSet = RControllerSet (new ControllerSet());

    KnobController cvar_volume (16, 0);
    RController volume (new ControllableLevelLinear (0.1));
    masterVolume = volume;
    controlSet->addController (volume,16,0,"masterVolume");

    phone = RPhone (new Phone (controlSet, volume));
    
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
      phone->masterEnvelope->noteOn(t, (double)velocity/127.0);
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
      phone->masterEnvelope->noteOff(t);
  }

  int numNotesOn;
  vector<bool> notesOn;


};

class PolySynthesiser : public boost::mutex
{
 public:
  RControllerSet controlSet;
  RController masterVolume;

  double t = 0;

  ofstream recording;

  vector<RPhone> phones;
  vector<RPhone> notesOn;
  vector<double> timePhoneStarted;

  map<RPhone,int> noteBeingPlayed;

  PolySynthesiser ();

  void save(string filename);
  void load(string filename);
  void interpretControlMessage (KnobController con, int val);
  void playToBuffer (RBufferWithMutex buffer);
  void noteOn (int noteVal, int velocity);
  void noteOff (int noteVal, int velocity);
 
};
