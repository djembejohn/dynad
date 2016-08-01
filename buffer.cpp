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

#include "buffer.h"

ExponentialLookUpTable exponentialLookUpTable;
FrequencyModulatorExponentialLookUpTable frequencyModulatorExponentialLookUpTable;
GaussianNoiseBuffer noisegen;
NoiseBuffer whitenoisegen;
SinWaveLookupTable sinWaveLookupTable;

#if 0
float & operator[] (const BufferWithMutex & buffer, const size_t index)
{
  if (index >= buffer.bufferSize) {
    throw "Overreached buffer size!";
  }
  else
    return buffer.outbuf[index]
}
#endif
