//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2024
// filename: fft_filter.cpp
// description:
// License: MIT
//

#include "fft_filter.h"
#include "fft.h"
#include <cmath>
#include <cstdio>

#ifndef SIMULATION
#include "pico/stdlib.h"
#endif


#ifndef SIMULATION
void __not_in_flash_func(fft_filter::filter_block)(int16_t sample_real[], int16_t sample_imag[], uint16_t start_bin, uint16_t stop_bin, bool lower_sideband, bool upper_sideband) {
#else
void fft_filter::filter_block(int16_t sample_real[], int16_t sample_imag[], uint16_t start_bin, uint16_t stop_bin, bool lower_sideband, bool upper_sideband) {
#endif

  // window
  for (uint16_t i = 0; i < fft_size; i++) {
    sample_real[i] = product(sample_real[i], window[i]);
    sample_imag[i] = product(sample_imag[i], window[i]);
  }

  // forward FFT
  fixed_fft(sample_real, sample_imag, 8);

  //DC and positive frequencies
  for (uint16_t i = 0; i < (new_fft_size/2u) + 1; i++) {
    //clear bins outside pass band
    if(!upper_sideband || i < start_bin || i > stop_bin)
    {
      sample_real[i] = 0;
      sample_imag[i] = 0;
    }
    else
    {
      sample_real[i] = sample_real[i];
      sample_imag[i] = sample_imag[i];
    }
  }

  //negative frequencies
  for (uint16_t i = 0; i < (new_fft_size/2u)-1; i++) {
    uint16_t bin = new_fft_size/2 - i - 1;
    if(!lower_sideband || bin < start_bin || bin > stop_bin)
    {
      sample_real[(new_fft_size/2u) + 1 + i] = 0;
      sample_imag[(new_fft_size/2u) + 1 + i] = 0;
    }
    else
    {
      sample_real[(new_fft_size/2u) + 1 + i] = sample_real[fft_size - (new_fft_size/2u) + i + 1];
      sample_imag[(new_fft_size/2u) + 1 + i] = sample_imag[fft_size - (new_fft_size/2u) + i + 1];
    }
  }


  // inverse FFT
  fixed_ifft(sample_real, sample_imag, 7);

}


#ifndef SIMULATION
void __not_in_flash_func(fft_filter::process_sample)(int16_t sample_real[], int16_t sample_imag[], uint16_t start_bin, uint16_t stop_bin, bool lower_sideband, bool upper_sideband) {
#else
void fft_filter::process_sample(int16_t sample_real[], int16_t sample_imag[], uint16_t start_bin, uint16_t stop_bin, bool lower_sideband, bool upper_sideband) {
#endif

  int16_t real[fft_size];
  int16_t imag[fft_size];

  for (uint16_t i = 0; i < (fft_size/2u); i++) {
    real[i] = last_input_real[i];
    imag[i] = last_input_imag[i];
    real[fft_size/2u + i] = sample_real[i];
    imag[fft_size/2u + i] = sample_imag[i];
    last_input_real[i] = sample_real[i];
    last_input_imag[i] = sample_imag[i];
  }

  //filter combined block
  filter_block(real, imag, start_bin, stop_bin, lower_sideband, upper_sideband);

  for (uint16_t i = 0; i < (new_fft_size/2u); i++) {
    sample_real[i] = real[i] + last_output_real[i];
    sample_imag[i] = imag[i] + last_output_imag[i];
    last_output_real[i] = real[new_fft_size/2u + i];
    last_output_imag[i] = imag[new_fft_size/2u + i];
  }

}
