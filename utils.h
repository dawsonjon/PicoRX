#ifndef _utils_
#define _utils_

#include <cstdint>

extern int16_t sin_table[2048];
//from: http://dspguru.com/dsp/tricks/magnitude-estimator/
uint16_t rectangular_2_magnitude(int16_t i, int16_t q);
//from: https://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization/
int16_t rectangular_2_phase(int16_t i, int16_t q);

void initialise_luts();

#endif
