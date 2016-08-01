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
#include "buffer.h"

typedef shared_ptr<class Oscillator> ROscillator;

class Oscillator
{
 public:
  
  SmoothedController level;


  RController noteFrequency;
  RController frequencyMultiplier;

  RController distortion;
  RController squareness;
  RController sawness;

  RController chorus;

  RController frequencyNoiseLevel;
  RController frequencyNoiseRate;

  RController amplitudeNoiseLevel;
  RController amplitudeNoiseRate; 
  

  ROscillator frequencyOscillator;
  ROscillator amplitudeOscillator;


  Oscillator (RController _level, RController _frequencyMultiplier, 
	      RController _noteFrequency = 0, RController _frequencyNoiseLevel = 0, 
	      RController _distortion = 0, RController _squareness = 0, 
	      RController _sawness = 0, RController _chorus = 0)
    : level(_level,0.001), noteFrequency (_noteFrequency), 
    frequencyMultiplier (_frequencyMultiplier), 
    frequencyNoiseLevel(_frequencyNoiseLevel), distortion (_distortion),
    squareness (_squareness), sawness(_sawness), chorus(_chorus),
    frequencyOscillator(0), amplitudeOscillator(0),
    previous_t(0.0), 
    internal_phase (whitenoisegen.get()*2*local_pi), 
    internal_phase_chorus(whitenoisegen.get()*2*local_pi),
    frequencyNoisepos(0.0)
    {
      target_frequency = 1.0;
      if (noteFrequency)
	target_frequency = target_frequency *noteFrequency->getOutputLevel();
      target_frequency =  target_frequency*frequencyMultiplier->getOutputLevel();
      internal_frequency = target_frequency;
      if (chorus)
	internal_frequency_chorus = target_frequency+chorus->getOutputLevel();
      else
	internal_frequency_chorus = target_frequency;
    }


  bool attachController (RController con) {
    string name = con->getName();
    if (name.find("_") != string::npos) 
      name = name.substr(0,name.find("_"));

    if (name == "frequencyNoiseLevel")
      frequencyNoiseLevel = con;
    else if (name == "frequencyNoiseRate")
      frequencyNoiseRate = con;
    else if (name == "amplitudeNoiseLevel")
      amplitudeNoiseLevel = con;
    else if (name == "amplitudeNoiseRate")
      amplitudeNoiseRate = con;
    else if (name == "distortion")
      distortion = con;
    else if (name == "squareness")
      squareness = con;
    else if (name == "sawness")
      sawness = con;
    else if (name == "chorus")
      chorus = con;
    else
      return false;

    return true;

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
    
    if (frequencyNoiseLevel) {
      frequencyNoisepos -= frequencyNoisepos*0.01;
      if (frequencyNoiseLevel->getOutputLevel() > 0) {
      double frequencyNoise_level = frequencyNoiseLevel->getOutputLevel()*frequencyNoiseLevel->getOutputLevel();
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
    // this version just has a square wave and interpolates toward it
    double value = sinWaveLookupTable.getValue(local_phase);

    if (squareness) {
      double squareness_val = squareness->getOutputLevel();
      if (squareness_val > 0) {
	double square_wave_value = local_phase<local_pi ? 1.0 : -1.0;
	value = (1.0-squareness_val)*value + squareness_val*square_wave_value;
      }
    }

    if (sawness) {
      double sawness_val = sawness->getOutputLevel();
      if (sawness_val > 0) {
	double saw_wave_value = 1.0-local_phase/local_pi;
	value = (1.0-sawness_val)*value + sawness_val*saw_wave_value;
      }
    }


    if (chorus) {
      if (chorus->getOutputLevel() != 1.0) {
	double target_frequency_chorus = target_frequency * chorus->getOutputLevel();
	internal_frequency_chorus =  0.7*target_frequency_chorus + 0.3*internal_frequency_chorus;
	internal_phase_chorus += 2*local_pi*internal_frequency_chorus*t_advance;

	while (internal_phase_chorus > 2*local_pi)
	  internal_phase_chorus -= 2*local_pi;
	
	double local_phase_chorus = internal_phase_chorus;
	// this version just has a square wave and interpolates toward it
	double value_chorus = sinWaveLookupTable.getValue(local_phase_chorus);

	if (squareness) {
	  double squareness_val = squareness->getOutputLevel();
	  if (squareness_val > 0) {
	    double square_wave_value_chorus = local_phase_chorus<local_pi ? 1.0 : -1.0;
	    value_chorus = (1.0-squareness_val)*value_chorus + squareness_val*square_wave_value_chorus;
	  }
	}

	if (sawness) {
	  double sawness_val = sawness->getOutputLevel();
	  if (sawness_val > 0) {
	    double saw_wave_value_chorus = 1.0-local_phase_chorus/local_pi;
	    value_chorus = (1.0-sawness_val)*value_chorus + sawness_val*saw_wave_value_chorus;
	  }
	}
	
	value = value*0.71 + value_chorus*0.71;
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

    if (amplitudeNoiseLevel && amplitudeNoiseRate) if (amplitudeNoiseLevel->getOutputLevel() >0) {
	if (t > tPreviousAmplitudeChange+amplitudeNoiseRate->getOutputLevel()){
	  amplitudeNoise = whitenoisegen.get()*2.0;
	  tPreviousAmplitudeChange = t;
	}
	thisLevel = thisLevel * amplitudeNoise;
      }

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
  double internal_phase_chorus;
  double internal_frequency;
  double internal_frequency_chorus;
  double target_frequency;
  double frequencyNoisepos;

  double targetLevel;
  double internalLevel;

  double tPreviousAmplitudeChange;
  double amplitudeNoise;

  static int newID;
};
