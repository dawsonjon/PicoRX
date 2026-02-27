#ifndef FFT_H_
#define FFT_H_
#include <cstdint>
#include <cmath>

const uint8_t fraction_bits = 14;
const int16_t K  =  (1 << (fraction_bits - 1));

void fft_initialise();
void fixed_fft(int16_t reals[], int16_t imaginaries[], unsigned m, bool scale=true);
void fixed_ifft(int16_t reals[], int16_t imaginaries[], unsigned m);

static inline int16_t float2fixed(float float_value) {
        return round(float_value * (1 << fraction_bits));
}

static inline int16_t product(int16_t a, int16_t b) {
        return ((static_cast<int32_t>(b) * a)+K) >> fraction_bits;
}

#endif
