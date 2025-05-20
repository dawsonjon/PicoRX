#ifndef _utils_
#define _utils_

#include <cstdint>

extern int16_t sin_table[2048];

uint16_t rectangular_2_magnitude(int16_t i, int16_t q);
void rectangular_2_polar(int16_t i, int16_t q, uint16_t *mag, int16_t *phase);
void initialise_luts();

#endif
