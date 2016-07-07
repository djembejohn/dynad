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

#include "jabVector.h"
#include "counted.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/thread/mutex.hpp>
#include "Stk.h"

using namespace std;
using namespace stk;
using namespace jab;

typedef boost::mt19937 base_rng_type;
typedef boost::mt19937& base_rng_ref;
typedef boost::normal_distribution<double> norm_dist;  
typedef boost::variate_generator<base_rng_ref,norm_dist> norm_gen; 

const double local_pi = 3.14159265358979323846;

extern class GaussianNoiseBuffer noisegen;
extern class SinWaveLookupTable sinWaveLookupTable;
extern class ExponentialLookUpTable exponentialLookUpTable;
extern class FrequencyModulatorExponentialLookUpTable frequencyModulatorExponentialLookUpTable;

template<class T=StkFloat>
class BufferWithMutex : public boost::mutex, public PCountedHeapObject
{
 public:
 BufferWithMutex(double _timeStart, double _sampleRate, 
		 unsigned int _numberOfFrames, 
		 unsigned int _numberOfChannels)
   : timeStart(_timeStart), sampleRate (_sampleRate),
    numberOfFrames(_numberOfFrames), numberOfChannels(_numberOfChannels),
 bufferSize (numberOfFrames*numberOfChannels), outbuf (bufferSize,0)
  {}
  
  double timeStart;
  double sampleRate;
  unsigned int numberOfFrames;
  unsigned int numberOfChannels;
  
  T & at (unsigned int index) {
    return outbuf.at(index);
  }

  safeVector<T> & getOutBufRef() {
    return outbuf;
  }

 private:

  size_t bufferSize;

  safeVector<T> outbuf;
};

typedef shared_ptr<BufferWithMutex<> > RBufferWithMutex;


class ExponentialLookUpTable
{
 public:

  ExponentialLookUpTable () 
    : bufferSize(4096)
  {
    // so that proportions can be between 0 and 1, I need to make the
    // buffer size one bigger
    buffer.resize(bufferSize+1,0.0);
    inverse.resize(bufferSize+1,0.0);
    double curve = 5.0;
    for (double i = 0; i< bufferSize+1; i+= 0.1) {
      double x = i/double(bufferSize);
      double y = (exp(-curve*x)-exp(-curve))/(1.0-exp(-curve));
      if (i-int(i) <0.1) 
	buffer[int(i)] = y;
      int yi = int(y*bufferSize);
      //      double inverse_y = -log(x*(1.0-exp(-curve)+exp(-curve)))/curve;
      inverse[yi] = x;
    }
  }

  double getValue(double proportion){
    if (proportion<0) 
      proportion = 0;
    if (proportion>1)
      proportion = 1;
    uint64_t bufferLocation = uint64_t(double(bufferSize)*proportion);
    return buffer[bufferLocation];
  }

  double getInverseValue(double value){
    if (value<0) 
      value = 0;
    if (value>1)
      value = 1;
    uint64_t bufferLocation = uint64_t(double(bufferSize)*value+0.5);
    return inverse[bufferLocation];
  }


  // Shape _/

  double getValueConcaveIncreasing (double proportion) {
    return (getValue(1.0-proportion));
  }

  double getInverseValueConcaveIncreasing (double value) {
    return (1.0-getInverseValue(value));
  }


  // Shape  _
  //       /

  double getValueConvexIncreasing (double proportion) {
    return (1.0-getValue(proportion));
  }
  
  // Shape \_
  
  double getValueConcaveDecreasing (double proportion) {
    return getValue(proportion);
  }

  double getInverseValueConcaveDecreasing (double value) {
    return getInverseValue(value);
  }
  
  // Shape _
  //        |
  //
  double getValueConvexDecreasing (double proportion) {
    return (1.0-getInverseValue(1.0-proportion));
  }

  double getInverseValueConvexDecreasing (double value) {
    return (1.0-getValue(1.0-value));
  }
  
  
 private:
  
  const size_t bufferSize;
  safeVector<double> buffer;
  safeVector<double> inverse;
  
};

class FrequencyModulatorExponentialLookUpTable
{
 public:

  FrequencyModulatorExponentialLookUpTable () 
    : bufferSize(1024)
  {
    // so that values can be between -1 and 1, I need to make the
    // buffer size one bigger
    buffer.resize(bufferSize+1,0.0);
    double curve=1.0;
    for (size_t i = 0; i< bufferSize+1; i++) {
      double x = 2.0*double(i)/double(bufferSize)-1.0;
      buffer[i] = (exp(-curve*x)-exp(-curve))/(1.0-exp(-curve));
    }
  }

  double getValue(double input){
    if (input<-1) 
      input = -1;
    if (input>1)
      input = 1;
    uint64_t bufferLocation = uint64_t(double(bufferSize)*((input+1.0)/2.0));
    return buffer[bufferLocation];
  }
 private:
  
  const size_t bufferSize;
  safeVector<double> buffer;
  
};



class GaussianNoiseBuffer
{
 public:
  safeVector<double> buffer;
  size_t counter;
  base_rng_type RNG;
  
  GaussianNoiseBuffer() 
    : counter (0)
  {
    norm_gen gen (RNG,norm_dist(0.0,1.0));
    buffer.reserve(100000);
    while (buffer.size()<=100000)
      buffer.push_back(gen());
  }
  
  double get() {
    counter ++;
    if (counter >= buffer.size())
      counter = 0;
    return buffer[counter];
  }
};


class SinWaveLookupTable
{
public:

  SinWaveLookupTable () 
    : bufferSize(16384)
  {
    buffer.resize(bufferSize+1,0.0);
    for (size_t i = 0; i< bufferSize; i++) 
      buffer[i] = sin(2.0*local_pi*double(i)/double(bufferSize));
    inverse.resize(bufferSize,0.0);
    for (size_t i = 0; i< bufferSize; i++) 
      inverse[i] = asin(double(2.0*i-1)/double(bufferSize));
  }
  
  double getValue(double t){
    //    if (t>2.0*local_pi || t<0)
    //      t = fmod(t,2.0*local_pi);
    int bufferLocation = int(double(bufferSize)*t/(2.0*local_pi)+0.5);
    
    //    cout << t << " " << bufferLocation << " " << bufferLocation % bufferSize << endl;
    
    while (bufferLocation >= (int)bufferSize)
      bufferLocation -= bufferSize;

    while (bufferLocation <0 )
      bufferLocation += bufferSize;
    
    return buffer[bufferLocation];
  }
  
 private:
  
  const size_t bufferSize;
  safeVector<double> buffer;
  safeVector<double> inverse;
  
};

