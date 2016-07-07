// This file is part of Dynad.

//    Dynad is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    Dynad is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with Dynad.  If not, see <http://www.gnu.org/licenses/>.

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
