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

#include "fft.h"
#include "rx_definitions.h"

struct s_filter_control
{
  uint16_t start_bin; 
  uint16_t stop_bin; 
  int16_t fft_bin;
  int8_t noise_smoothing;
  int8_t noise_threshold;
  uint8_t spectrum_smoothing;
  bool lower_sideband; 
  bool upper_sideband; 
  bool capture;
  bool enable_auto_notch;
  bool enable_noise_reduction;
};

class fft_filter
{

  int16_t last_input_real[fft_size/2u];
  int16_t last_input_imag[fft_size/2u];
  int16_t last_output_real[new_fft_size/2u];
  int16_t last_output_imag[new_fft_size/2u];
  int32_t positive_noise_estimate[new_fft_size/2u];
  int16_t positive_signal_estimate[new_fft_size/2u];
  int32_t negative_noise_estimate[new_fft_size/2u];
  int16_t negative_signal_estimate[new_fft_size/2u];
  int32_t window[fft_size];
  void filter_block(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture[]);

  public:
  fft_filter()
  {
    fft_initialise();
    for (uint16_t i = 0; i < fft_size; i++) {
      const float multiplier = 0.5 * (1 - cosf(2 * M_PI * i / (fft_size - 1)));
      window[i] = float2fixed(multiplier);
    }
    for (uint16_t i = 0; i < fft_size/2u; i++) {
      last_input_real[i] = 0;
      last_input_imag[i] = 0;
    }
    for (uint16_t i = 0; i < new_fft_size/2; i++) {
      last_output_real[i] = 0;
      last_output_imag[i] = 0;
      positive_noise_estimate[i] = INT32_MAX-1;
      positive_signal_estimate[i] = 0;
      negative_noise_estimate[i] = INT32_MAX-1;
      negative_signal_estimate[i] = 0;
    }
  }
  void process_sample(int16_t sample_iq[], s_filter_control &filter_control, int16_t capture[]);

};

#endif
