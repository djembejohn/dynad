#include "buffer.h"

ExponentialLookUpTable exponentialLookUpTable;
FrequencyModulatorExponentialLookUpTable frequencyModulatorExponentialLookUpTable;
GaussianNoiseBuffer noisegen;
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
