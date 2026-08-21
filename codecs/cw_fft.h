//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: fft.h
// description: Fast Fixed-Point FFT
// License: MIT
//

#ifndef CW_FFT_H_
#define CW_FFT_H_
#include <cstdint>

void cw_fft_initialise();
void cw_fixed_fft(int16_t reals[], int16_t imaginaries[], unsigned m);
void cw_fixed_ifft(int16_t reals[], int16_t imaginaries[], unsigned m);
int16_t cw_float2fixed(float float_value);
int16_t cw_product(int16_t a, int16_t b);

#endif
