#pragma once


class MultiPartial
{
 public:

  list <ROscillator> oscillators;

  list<REnvelope> envelopes;
  RController masterVolume;


  // need to code up how the frequencies multiply and add two? shape
  // controllers, one for the frequency mulipliers and one for the
  // amplitude shape... which could just be how sharp the exponential
  // rate of decline is
  MultiPartial (int numSubPartials, RController _masterVolume, RController level, RNoteControlledFrequency noteFrequency, RControllableFrequencyMultiplier frequencyMultiplier, RController frequencyNoise = 0, RController distortion = 0, RController shape = 0)
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

  void playToBuffer (RBufferWithMutex buffer, double tStart) 
  {
    
    float * outbuf = buffer->getOutBuf();

    for (unsigned int i = 0; i<buffer->numberOfFrames; i++) {
      double timePoint = tStart+(double)i/(double)buffer->sampleRate;
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
      outbuf[i*buffer->numberOfChannels] += value * level;
    }
  }
    
};
