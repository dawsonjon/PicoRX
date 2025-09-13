//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2024
// filename: fft_filter.cpp
// description:
// License: MIT
//

#include <cmath>
#include <cstdio>
#include <algorithm>

#include "fft_filter.h"
#include "fft.h"
#include "utils.h"
#include "noise_reduction.h"
#include "cic_corrections.h"
#include "fft.h"
#include "utils.h"

#ifndef SIMULATION
#include "pico/stdlib.h"
#endif

#ifndef SIMULATION
void __not_in_flash_func(fft_filter::filter_block)(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture[]) {
#else
void fft_filter::filter_block(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture[]) {
#endif

  // window
  for (uint16_t i = 0; i < fft_size; i++) {
    sample_real[i] = product(sample_real[i], window[i]);
    sample_imag[i] = product(sample_imag[i], window[i]);
  }

  // forward FFT
  fixed_fft(sample_real, sample_imag, 8);

  if(filter_control.capture)
  {
    for (uint16_t i = 0; i < fft_size; i++) {
      capture[i] = (((int32_t)capture[i]<<filter_control.spectrum_smoothing) - capture[i] + rectangular_2_magnitude(sample_real[i], sample_imag[i])) >> filter_control.spectrum_smoothing;
    }
  }

  //largest bin
  int16_t peak = 0;
  int16_t next_peak = 0;
  uint16_t peak_bin = 0;

  //DC and positive frequencies
  for (uint16_t i = 0; i < (new_fft_size/2u) + 1; i++) {
    //clear bins outside pass band
    if(!filter_control.upper_sideband || i < filter_control.start_bin || i > filter_control.stop_bin)
    {
      sample_real[i] = 0;
      sample_imag[i] = 0;
    }
    else
    {
      sample_real[i] = cic_correct(i, filter_control.fft_bin, sample_real[i]);
      sample_imag[i] = cic_correct(i, filter_control.fft_bin, sample_imag[i]);

      //capture highest and second highest peak
      uint16_t magnitude = rectangular_2_magnitude(sample_real[i], sample_imag[i]);
      if(magnitude > peak)
      {
        peak = magnitude; 
        peak_bin = i;
      }
      else if(magnitude > next_peak)
      {
        next_peak = magnitude;
      }

    }
  }

  //apply noise filtering to DC and positive frequencies
  if(filter_control.enable_noise_reduction && filter_control.upper_sideband)
  {
    const uint16_t start_bin = std::max((uint16_t)4, filter_control.start_bin);
    noise_reduction(
      sample_real, 
      sample_imag, 
      positive_noise_estimate, 
      positive_signal_estimate, 
      start_bin, 
      filter_control.stop_bin,
      filter_control.noise_smoothing,
      filter_control.noise_threshold);
  }

  //negative frequencies
  for (uint16_t i = 0; i < (new_fft_size/2u)-1; i++) {
    const uint16_t bin = new_fft_size/2 - i - 1;
    const uint16_t new_idx = (new_fft_size/2u) + 1 + i;
    if(!filter_control.lower_sideband || bin < filter_control.start_bin || bin > filter_control.stop_bin)
    {
      sample_real[new_idx] = 0;
      sample_imag[new_idx] = 0;
    }
    else
    {
      sample_real[new_idx] = cic_correct(bin, filter_control.fft_bin, sample_real[fft_size - (new_fft_size/2u) + i + 1]);
      sample_imag[new_idx] = cic_correct(bin, filter_control.fft_bin, sample_imag[fft_size - (new_fft_size/2u) + i + 1]);

      //capture highest and second highest peak
      uint16_t magnitude = rectangular_2_magnitude(sample_real[new_idx], sample_imag[new_idx]);
      if(magnitude > peak)
      {
        peak = magnitude; 
        peak_bin = new_idx;
      }
      else if(magnitude > next_peak)
      {
        next_peak = magnitude;
      }
    }
  }

  //apply noise filtering to negative frequencies
  if(filter_control.enable_noise_reduction && filter_control.lower_sideband)
  {
    const uint16_t start_bin = std::max((uint16_t)2, filter_control.start_bin);
    noise_reduction(
      &sample_real[new_fft_size/2u], 
      &sample_imag[new_fft_size/2u], 
      negative_noise_estimate, 
      negative_signal_estimate, 
      new_fft_size/2u-1-filter_control.stop_bin, 
      new_fft_size/2u-1-start_bin,
      filter_control.noise_smoothing,
      filter_control.noise_threshold);
  }

  if(filter_control.enable_auto_notch)
  {
    //check for a consistent peak
    const uint8_t confirm_threshold = 255u;
    static uint8_t confirm_count = 0u;
    static uint8_t last_peak_bin = 0u;
    if(peak_bin == last_peak_bin && confirm_count < confirm_threshold) confirm_count++;
    if(peak_bin != last_peak_bin && confirm_count > 0) confirm_count--;
    last_peak_bin = peak_bin;

    //remove highest bin
    if((confirm_count > confirm_threshold/2u) && (peak_bin > 3u) && (peak_bin < new_fft_size-3u))
    {
      sample_real[peak_bin] = 0;
      sample_imag[peak_bin] = 0;
      sample_real[peak_bin+1] = 0;
      sample_imag[peak_bin+1] = 0;
      sample_real[peak_bin-1] = 0;
      sample_imag[peak_bin-1] = 0;
    }
  }

  // inverse FFT
  fixed_ifft(sample_real, sample_imag, 7);

}


#ifndef SIMULATION
void __not_in_flash_func(fft_filter::process_sample)(int16_t sample_iq[], s_filter_control &filter_control, int16_t capture[]) {
#else
void fft_filter::process_sample(int16_t sample_iq[], s_filter_control &filter_control, int16_t capture[]) {
#endif

  int16_t real[fft_size];
  int16_t imag[fft_size];

  for (uint16_t i = 0; i < (fft_size/2u); i++) {
    real[i] = last_input_real[i];
    imag[i] = last_input_imag[i];
    real[fft_size/2u + i] = sample_iq[2 * i];
    imag[fft_size/2u + i] = sample_iq[2 * i + 1];
    last_input_real[i] = sample_iq[2 * i];
    last_input_imag[i] = sample_iq[2 * i + 1];
  }

  //filter combined block
  filter_block(real, imag, filter_control, capture);

  for (uint16_t i = 0; i < (new_fft_size/2u); i++) {
    sample_iq[2 * i] = real[i] + last_output_real[i];
    sample_iq[2 * i + 1] = imag[i] + last_output_imag[i];
    last_output_real[i] = real[new_fft_size/2u + i];
    last_output_imag[i] = imag[new_fft_size/2u + i];
  }

}
