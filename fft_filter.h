//  _  ___  _   _____ _     _                 
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___ 
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/    
//
// Copyright (c) Jonathan P Dawson 2024
// filename: fft_filter.h
// description:
// License: MIT
//

#ifndef FFT_FILTER_H
#define FFT_FILTER_H
#include <stdint.h>
#include <cmath>

#include "fft.h"

static const uint16_t fft_size = 256;
static const uint16_t new_fft_size = fft_size/2; 

class fft_filter
{

  int16_t last_input_real[fft_size/2u];
  int16_t last_input_imag[fft_size/2u];
  int16_t last_output_real[new_fft_size/2];
  int16_t last_output_imag[new_fft_size/2];
  int32_t window[fft_size];
  void filter_block(int16_t sample_real[], int16_t sample_imag[], uint16_t start_bin, uint16_t stop_bin, bool lower_sideband, bool upper_sideband);

  public:
  fft_filter()
  {
    fft_initialise();
    for (int i = 0; i < fft_size; i++) {
      float multiplier = 0.5 * (1 - cosf(2*M_PI*i/fft_size));
      window[i] = float2fixed(multiplier);
    }
    for (int i = 0; i < fft_size/2u; i++) {
      last_input_real[i] = 0;
      last_input_imag[i] = 0;
    }
    for (int i = 0; i < new_fft_size/2; i++) {
      last_output_real[i] = 0;
      last_output_imag[i] = 0;
    }
  }
  void process_sample(int16_t sample_real[], int16_t sample_imag[], uint16_t start_bin, uint16_t stop_bin, bool lower_sideband, bool upper_sideband);

};

#endif
