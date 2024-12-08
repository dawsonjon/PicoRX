#include <cstdint>
#include <algorithm>
#include <iostream>

#include "utils.h"
#include "noise_reduction.h"

const int8_t magnitude_smoothing = 3;
const int8_t noise_smoothing = 10;
const int16_t threshold = 1;
const uint8_t fraction_bits = 14u;
const int32_t scaling = 1<<fraction_bits;


void noise_reduction(int16_t i[], int16_t q[], int32_t noise_estimate[], int16_t signal_estimate[], uint16_t start, uint16_t stop)
{
    for(uint16_t idx = start; idx <= stop; ++idx)
    {

      const int32_t magnitude = rectangular_2_magnitude(i[idx], q[idx]);
      int32_t signal_level = signal_estimate[idx];
      int32_t noise_level = noise_estimate[idx];

      signal_level = ((signal_level << magnitude_smoothing) + (magnitude - signal_level)) >> magnitude_smoothing;
      noise_level = std::min(noise_level+1, signal_level << noise_smoothing);

      int32_t gain = 0;
      if(signal_level > 0)
      {
        gain = scaling-(threshold*scaling*(noise_level>>noise_smoothing)/signal_level);
        gain = std::min(std::max(gain, (int32_t)0), scaling);
      } 
      
      signal_estimate[idx] = signal_level;
      noise_estimate[idx] = noise_level;
      i[idx] = (i[idx]*gain)>>fraction_bits;
      q[idx] = (q[idx]*gain)>>fraction_bits;

    }
}
