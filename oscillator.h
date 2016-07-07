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
#include "buffer.h"

typedef shared_ptr<class Oscillator> ROscillator;

class Oscillator
{
 public:
  
  SmoothedController level;
  RController noteFrequency;
  RController frequencyMultiplier;
  RController frequencyNoise;
  RController distortion;
  RController squareness;
  RController sawness;
  
  ROscillator frequencyOscillator;
  ROscillator amplitudeOscillator;

  Oscillator (RController _level, RController _frequencyMultiplier, 
	      RController _noteFrequency = 0, RController _frequencyNoise = 0, 
	      RController _distortion = 0, RController _squareness = 0, 
	      RController _sawness = 0)
    : level(_level,0.001), noteFrequency (_noteFrequency), 
    frequencyMultiplier (_frequencyMultiplier), 
    frequencyNoise(_frequencyNoise), distortion (_distortion),
    squareness (_squareness), sawness(_sawness),
    frequencyOscillator(0), amplitudeOscillator(0),
    previous_t(0.0), internal_phase (0.0), 
    frequencyNoisepos(0.0)
    {
      target_frequency = 1.0;
      if (noteFrequency)
	target_frequency = target_frequency *noteFrequency->getOutputLevel();
      target_frequency =  target_frequency*frequencyMultiplier->getOutputLevel();
      internal_frequency = target_frequency;
    }
  
  //  Oscillator (double _frequency, double _level, double _noise=0, double _phase = 0) 
  //    : baseFrequency (_frequency), frequency (_frequency), level (_level), noise(_noise), phase (_phase), previous_t(0), position(phase)
  //  {}

  double getValue(double t) {
    double t_advance = t-previous_t;
    previous_t = t;

    double internalLevel = level.getOutputLevel();
    if (internalLevel == 0)
      return 0;
    
    target_frequency = 1.0;
    if (noteFrequency)
      target_frequency = target_frequency *noteFrequency->getOutputLevel();
    target_frequency =  target_frequency*frequencyMultiplier->getOutputLevel();
    
    
    if (frequencyOscillator) if (frequencyOscillator->level.getOutputLevel() > 0)
      target_frequency = target_frequency*frequencyModulatorExponentialLookUpTable.getValue (frequencyOscillator->getValue(t));
    
    if (frequencyNoise) {
      frequencyNoisepos -= frequencyNoisepos*0.01;
      if (frequencyNoise->getOutputLevel() > 0) {
      double frequencyNoise_level = frequencyNoise->getOutputLevel()*frequencyNoise->getOutputLevel();
	frequencyNoisepos += frequencyNoise_level*noisegen.get();
	target_frequency += frequencyNoisepos;
      }
    }

    // a bit of smoothing of the frequency
    internal_frequency = 0.7*target_frequency + 0.3*internal_frequency;
    internal_phase += 2*local_pi*internal_frequency*t_advance;
    while (internal_phase > 2*local_pi)
      internal_phase -= 2*local_pi;

    double local_phase = internal_phase;

#if 0
    // this is something I dreamed up which makes the waveform squarer
    // or pointier - it does add in some harmonics but hey ho
    if (squareness) if (squareness->getOutputLevel()>0 && squareness->getOutputLevel() != 0.5) {
      double inner_phase = 2.0*internal_phase;
      while (inner_phase>2*local_pi)
	inner_phase -= local_pi;
      while (inner_phase<0)
	inner_phase += local_pi;
      
      double squarenessval = 2.0*squareness->getOutputLevel()-1.0;
      local_phase = local_phase + squarenessval*sinWaveLookupTable.getValue(inner_phase);

      while (local_phase > 2*local_pi)
	local_phase -= 2*local_pi;
      while (local_phase <0)
	local_phase += 2*local_pi;

      //      cout << squareness->getOutputLevel() << " " 
      //	   << squarenessval << " " 
      //	   << internal_phase << " " << local_phase<< endl;
    }
    double value = sinWaveLookupTable.getValue(local_phase);
#else
    // this version just has a square wave and interpolates toward it

    double value = sinWaveLookupTable.getValue(local_phase);

    if (squareness) {
      double squareness_val = squareness->getOutputLevel();
      if (squareness_val > 0) {
	double square_wave_value = local_phase<local_pi ? 1.0 : -1.0;
	value = (1.0-squareness_val)*value + squareness_val*square_wave_value;
      }
    }
#endif

    if (sawness) {
      double sawness_val = sawness->getOutputLevel();
      if (sawness_val > 0) {
	double saw_wave_value = 1.0-local_phase/local_pi;
	value = (1.0-sawness_val)*value + sawness_val*saw_wave_value;
      }
    }


    if (distortion) {
      double distortionLevel = distortion->getOutputLevel();
      if (distortionLevel > 0.0) {
	if (value > 1.0-distortionLevel)
	  value = 1.0-distortionLevel;
	else if (value < -1.0 + distortionLevel)
	  value = -1.0+distortionLevel;

      }
    }
    double thisLevel = 1.0;
    if (amplitudeOscillator) if (amplitudeOscillator->level.getOutputLevel() > 0)
      thisLevel += amplitudeOscillator->getValue(t);
    //    if (frequencyMultiplier->getOutputLevel() == 1.0)
    //      cout << thisLevel << endl;
    value = value * thisLevel * internalLevel;
    return value;
  }

  // need to do something along the lines of knowing what phase 
  // we're in and then updating that when we get new ts etc

  double getInternalPhase() {
    return internal_phase;
  }
  
private:
  double previous_t;
  double internal_phase;
  double internal_frequency;
  double target_frequency;
  double frequencyNoisepos;
  static int newID;
};
