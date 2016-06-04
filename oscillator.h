#pragma once
#include "buffer.h"

typedef shared_ptr<class Oscillator> ROscillator;

class Oscillator
{
 public:
  
  RController level;
  RController noteFrequency;
  RController frequencyMultiplier;
  RController frequencyNoise;
  RController distortion;
  RController squareness;
  
  ROscillator frequencyOscillator;
  ROscillator amplitudeOscillator;

  Oscillator (RController _level, RController _frequencyMultiplier, RController _noteFrequency = 0, RController _frequencyNoise = 0, RController _distortion = 0, RController _squareness = 0)
    : level(_level), noteFrequency (_noteFrequency), 
    frequencyMultiplier (_frequencyMultiplier), 
    frequencyNoise(_frequencyNoise), distortion (_distortion),
    squareness (_squareness), 
    frequencyOscillator(0), amplitudeOscillator(0),
    previous_t(0.0), internal_phase (0.0), 
    frequencyNoisepos(0.0)
    {
      target_frequency = 1.0;
      if (noteFrequency)
	target_frequency = target_frequency *noteFrequency->getLevel();
      target_frequency =  target_frequency*frequencyMultiplier->getLevel();
      internal_frequency = target_frequency;
    }
  
  //  Oscillator (double _frequency, double _level, double _noise=0, double _phase = 0) 
  //    : baseFrequency (_frequency), frequency (_frequency), level (_level), noise(_noise), phase (_phase), previous_t(0), position(phase)
  //  {}

  double getValue(double t) {
    double t_advance = t-previous_t;
    previous_t = t;
    
    target_frequency = 1.0;
    if (noteFrequency)
      target_frequency = target_frequency *noteFrequency->getLevel();
    target_frequency =  target_frequency*frequencyMultiplier->getLevel();
    
    
    if (frequencyOscillator) if (frequencyOscillator->level > 0)
      target_frequency = target_frequency*frequencyModulatorExponentialLookUpTable.getValue (frequencyOscillator->getValue(t));
    
    if (frequencyNoise) {
      frequencyNoisepos -= frequencyNoisepos*0.01;
      if (frequencyNoise->getLevel() > 0) {
      double frequencyNoise_level = frequencyNoise->getLevel()*frequencyNoise->getLevel();
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
    if (squareness) if (squareness->getLevel()>0 && squareness->getLevel() != 0.5) {
      double inner_phase = 2.0*internal_phase;
      while (inner_phase>2*local_pi)
	inner_phase -= local_pi;
      while (inner_phase<0)
	inner_phase += local_pi;
      
      double squarenessval = 2.0*squareness->getLevel()-1.0;
      local_phase = local_phase + squarenessval*sinWaveLookupTable.getValue(inner_phase);

      while (local_phase > 2*local_pi)
	local_phase -= 2*local_pi;
      while (local_phase <0)
	local_phase += 2*local_pi;

      //      cout << squareness->getLevel() << " " 
      //	   << squarenessval << " " 
      //	   << internal_phase << " " << local_phase<< endl;
    }
    double value = sinWaveLookupTable.getValue(local_phase);
#else
    // this version just has a square wave and interpolates toward it

    double value = sinWaveLookupTable.getValue(local_phase);

    if (squareness) {
      double squareness_val = squareness->getLevel();
      if (squareness_val > 0) {
	double square_wave_value = local_phase<local_pi ? 1.0 : -1.0;
	value = (1.0-squareness_val)*value + squareness_val*square_wave_value;
      }
    }
#endif



    if (distortion) {
      double distortionLevel = distortion->getLevel();
      if (distortionLevel > 0.0) {
	if (value > 1.0-distortionLevel)
	  value = 1.0-distortionLevel;
	else if (value < -1.0 + distortionLevel)
	  value = -1.0+distortionLevel;

      }
    }
    double thisLevel = 1.0;
    if (amplitudeOscillator) if (amplitudeOscillator->level > 0)
      thisLevel += amplitudeOscillator->getValue(t);
    //    if (frequencyMultiplier->getLevel() == 1.0)
    //      cout << thisLevel << endl;
    value = value * thisLevel * level->getLevel();;
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
};
