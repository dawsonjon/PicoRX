//  _  ___  _   _____ _     _                 
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___ 
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
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

#ifdef PICORX_FFT_FILTER_USE_CMSIS
#include "pico/stdlib.h"
#include "arm_math.h"
#else
#include "fft.h"
#endif

static const uint16_t fft_size = 256;
static const uint16_t new_fft_size = fft_size/2; 

struct s_filter_control
{
  uint16_t start_bin; 
  uint16_t stop_bin; 
  bool lower_sideband; 
  bool upper_sideband; 
  bool capture;
  bool enable_auto_notch;
};

class fft_filter
{

  int16_t last_input_real[fft_size/2u];
  int16_t last_input_imag[fft_size/2u];
  int16_t last_output_real[new_fft_size/2];
  int16_t last_output_imag[new_fft_size/2];
#ifdef PICORX_FFT_FILTER_USE_CMSIS
  arm_cfft_instance_q15 fft_i;
  arm_cfft_instance_q15 ifft_i;
  q15_t window[fft_size];
#else
  int32_t window[fft_size];
#endif
  void filter_block(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture_i[], int16_t capture_q[]);

  public:
  fft_filter()
  {
#ifdef PICORX_FFT_FILTER_USE_CMSIS
    arm_status s = arm_cfft_init_256_q15(&fft_i);
    hard_assert(s == ARM_MATH_SUCCESS);
    s = arm_cfft_init_128_q15(&ifft_i);
    hard_assert(s == ARM_MATH_SUCCESS);
    for (uint16_t i = 0; i < fft_size; i++) {
      float multiplier = (1.0f - cosf(2*M_PI*i/fft_size));
      arm_float_to_q15(&multiplier, &window[i], 1);
    }
#else
    fft_initialise();
    for (uint16_t i = 0; i < fft_size; i++) {
      float multiplier = 0.5 * (1 - cosf(2*M_PI*i/fft_size));
      window[i] = float2fixed(multiplier);
    }
#endif
    for (uint16_t i = 0; i < fft_size/2u; i++) {
      last_input_real[i] = 0;
      last_input_imag[i] = 0;
    }
    for (uint16_t i = 0; i < new_fft_size/2; i++) {
      last_output_real[i] = 0;
      last_output_imag[i] = 0;
    }
  }
  void process_sample(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture_i[], int16_t capture_q[]);

};

#endif
