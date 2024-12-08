#ifndef __NOISE_REDUCTION_H__
#define __NOISE_REDUCTION_H__

#include <cstdint>
void noise_reduction(int16_t i[], int16_t q[], int32_t noise_estimate[], int16_t signal_estimate[], uint16_t start, uint16_t stop);

#endif
