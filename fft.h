#ifndef FFT_H_
#define FFT_H_
#include <cstdint>

void fft_initialise();
void fixed_fft(int16_t reals[], int16_t imaginaries[], unsigned m, bool scale=true);
void fixed_ifft(int16_t reals[], int16_t imaginaries[], unsigned m);
int16_t float2fixed(float float_value);
int16_t product(int16_t a, int16_t b);

#endif
