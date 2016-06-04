#pragma once

#include <list>
#include <map>
#include <memory>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/math/constants/constants.hpp>
#include "limits.h"
#include "jabVector.h"
//#include "counted.h"
#include "ofMain.h"
#include "envelope.h"

#include "controller.h"
#include "oscillator.h"

using namespace std;

// to do
//
// envelopes X
// amplitude oscillators
// buttons on bcr changing modes (easy version)
// different wave shapes
// saving controller sets
// morphing between controller sets
// oscillating between controller sets
// more partials
// get phone keyboard working
// midi back to bcr
// fine tuning partial freq
// tidy code into separate files
// polyphony?
// wav out?
// vst?
// upload to git
// show stuff on the screen


using namespace std;
using namespace jab;


extern double midiNoteToFrequency (int note);
extern double stringToFrequency (string note);
extern int stringToMidiNote (string note);

class BufferWithMutex {
 public:
  BufferWithMutex(ofSoundBuffer & _buffer)
    : buffer (_buffer) {}

  ofSoundBuffer & buffer;
  ofMutex mutex;
};


class MultiPartial
{
 public:

  list <ROscillator> oscillators;

  list<REnvelope> envelopes;
  RControllableLevel masterVolume;


  // need to code up how the frequencies multiply and add two? shape
  // controllers, one for the frequency mulipliers and one for the
  // amplitude shape... which could just be how sharp the exponential
  // rate of decline is
  MultiPartial (int numSubPartials, RControllableLevel _masterVolume, RControllableLevel level, RNoteControlledFrequency noteFrequency, RControllableFrequencyMultiplier frequencyMultiplier, RControllableLevel frequencyNoise = 0, RControllableLevel distortion = 0, RControllableLevel shape = 0)
    :  masterVolume(_masterVolume)
  {
    for (int i = 0; i< numSubPartials; i++) {
      oscillators.push_back(ROscillator(new Oscillator (level,frequencyMultiplier,noteFrequency,frequencyNoise,distortion,shape)));
    }
  }
  
  void addEnvelope (REnvelope env)
  {
    envelopes.push_back (env);
  }

  // The amplitude oscillators stack on top of one another, adding
  // together
  void setAmplitudeOscillator (shared_ptr<Oscillator> osc)
  {
    for (auto oscillator:oscillators)
      oscillator->amplitudeOscillator=osc;
  }

  void setFrequencyOscillator (shared_ptr<Oscillator> osc)
  {
    for (auto oscillator:oscillators)
      oscillator->frequencyOscillator=osc;
  }

  void playToBuffer (ofSoundBuffer & buffer, double tStart) 
  {
    
    for (unsigned int i = 0; i<buffer.getNumFrames(); i++) {
      double timePoint = tStart+(double)i/(double)buffer.getSampleRate();
      double level = masterVolume->getLevel();
      //      for (auto & osc:amp_oscillators) {
      //	level = level + osc.getValue(timePoint);
      //      }
      for (auto & env:envelopes) {
      	level = level * env->getLevel(timePoint);
      }
      double value = 0;
      for (auto oscillator:oscillators)
	value += oscillator->getValue(timePoint);
      // must implement pan ;)
      buffer[i*buffer.getNumChannels()] += value * level;
    }
  }
    
};

class Partial : public ofThread
{
public:
  
  Oscillator oscillator;

  list<REnvelope> envelopes;
  RControllableLevel masterVolume;

  Partial (RControllableLevel _masterVolume, RControllableLevel level, RNoteControlledFrequency noteFrequency, RControllableFrequencyMultiplier frequencyMultiplier, RControllableLevel frequencyNoise = 0, RControllableLevel distortion = 0, RControllableLevel shape = 0)
    : oscillator (level, frequencyMultiplier, noteFrequency, frequencyNoise, distortion, shape),
    masterVolume(_masterVolume)
    {}
    
  void threadedFunction() {
    // do nothing for now
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
  void playToBuffer (ofSoundBuffer & buffer, double tStart) 
  {
    
    for (unsigned int i = 0; i<buffer.getNumFrames(); i++) {
      double timePoint = tStart+(double)i/(double)buffer.getSampleRate();
      double level = masterVolume->getLevel();
      //      for (auto & osc:amp_oscillators) {
      //	level = level + osc.getValue(timePoint);
      //      }
      for (auto & env:envelopes) {
      	level = level * env->getLevel(timePoint);
      }
      double value = oscillator.getValue(timePoint);
      // must implement pan ;)
      buffer[i*buffer.getNumChannels()] += value * level;
    }
  }

};

typedef shared_ptr<Partial> RPartial;


class MonoSynthesiser
{
 public:
  list<RPartial> partials;
  map<KnobController, RController > controllers;

  REnvelope masterEnvelope;
  RNoteControlledFrequency masterNote;
  RControllableLevel masterVolume;

  double baseFrequency = 440.0;

  double t = 0;

  MonoSynthesiser () 
    {
      masterEnvelope = REnvelope(new Envelope (0.1,0.1,0.7,0.5));
      KnobController cvar_note (0, 0);
      RNoteControlledFrequency note (new NoteControlledFrequency (cvar_note,440));
      masterNote = note;
      KnobController cvar_volume (16, 0);
      RControllableLevel volume (new ControllableLevelLinear (cvar_volume,0.1));
      masterVolume = volume;

      controllers[cvar_note] = note;

      // need to think a bit more about how controllers work for notes
      // on and off etc

      // basically, need to implement a layer of controllers which can
      // be talked to by midi or OSC or anything really

      for (int i = 0; i< 8; i++) {
	
	KnobController cvar_row1(i,0);
	KnobController cvar_row2(i,1);

	KnobController cvar_row3(i,2);
	KnobController cvar_row4(i,3);

	KnobController cvar_set2row2(i,6);

	RControllableFrequencyMultiplier freqmult (new ControllableFrequencyMultiplier (cvar_row1,1.0*(i+1)));
	RControllableLevelConcaveIncreasing level (new ControllableLevelConcaveIncreasing (cvar_row2,exp(-i)));
	RControllableLevelLinear frequencyNoise (new ControllableLevelLinear (cvar_row2,0.00));
	RControllableLevelConcaveIncreasing distortion  (new ControllableLevelConcaveIncreasing (cvar_set2row2,0.0));
	RControllableLevelConcaveIncreasing shape (new ControllableLevelConcaveIncreasing (cvar_row4,0.0));

	controllers[cvar_row2] = level;
	controllers[cvar_row1] = freqmult;
	controllers[cvar_row3] = frequencyNoise;
	controllers[cvar_row4] = shape;
	controllers[cvar_set2row2] = distortion;
	// the above should all be possible on one line... make it so!

	RPartial newPartial = RPartial(new Partial (masterVolume, level, note, freqmult, frequencyNoise, distortion,shape));
	newPartial->addEnvelope (masterEnvelope);



	KnobController cvar_set3row1(i,10);
	KnobController cvar_set3row2(i,11);
	KnobController cvar_set3row3(i,12);
	KnobController cvar_set3row4(i,13);

	RControllableFrequencyLFO amposcfreq (new ControllableFrequencyLFO (cvar_set3row1,2.0));
	RControllableLevelConcaveIncreasing amposclevel (new ControllableLevelConcaveIncreasing (cvar_set3row2,0.0));

	RControllableFrequencyLFO freqoscfreq (new ControllableFrequencyLFO (cvar_set3row3,2.0));
	RControllableLevelConcaveIncreasing freqosclevel (new ControllableLevelConcaveIncreasing (cvar_set3row4,0.0));

	controllers[cvar_set3row1] = amposcfreq;
	controllers[cvar_set3row2] = amposclevel;
	controllers[cvar_set3row3] = freqoscfreq;
	controllers[cvar_set3row4] = freqosclevel;

	ROscillator amplitudeOscillator (new Oscillator (amposclevel,amposcfreq));
	ROscillator frequencyOscillator (new Oscillator(freqosclevel,freqoscfreq));
	newPartial->setAmplitudeOscillator(amplitudeOscillator);
	newPartial->setFrequencyOscillator(frequencyOscillator);


	partials.push_back (newPartial);
      }
    }
  
  // have to think about buffers and threading at some point

  void noteOn (int noteVal, int velocity) {
    masterEnvelope->noteOn(t, (double)velocity/127.0);
    masterNote->receiveMessage(noteVal);
  }

  void noteOff (int noteVal, int velocity) {
    masterEnvelope->noteOff(t);
  }

#if 0  
  void interpretControlMessage (KnobController cvar, int val) {
    map<KnobController, RController>::iterator mi = controllers.find(cvar);
    if (mi != controllers.end()) 
      mi->second->receiveMessage (val);
  }
#endif


  void interpretControlMessage (KnobController con, int val) {
    map<KnobController, RController>::iterator mi = controllers.find(con);
    if (mi != controllers.end()) 
      mi->second->receiveMessage (val);
  }

 
  void playToBuffer (ofSoundBuffer & buffer, vector<float> & showBuffer) {
    // the first partial is assumed to be the root
    RPartial firstPartial = *partials.begin();
    for (auto partial:partials)
      partial->playToBuffer(buffer,t);

    t += (double)buffer.getNumFrames()/(double)buffer.getSampleRate();

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
  
};
