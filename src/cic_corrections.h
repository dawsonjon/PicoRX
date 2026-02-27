#ifndef __CIC_CORRECTIONS__
#define __CIC_CORRECTIONS__
#include <cstdint>

extern const uint16_t cic_correction[];
int16_t cic_correct(int16_t fft_bin, int16_t fft_offset, int16_t sample);

#endif
