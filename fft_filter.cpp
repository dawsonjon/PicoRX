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

#include "fft_filter.h"
#ifndef PICORX_FFT_FILTER_USE_CMSIS
#include "fft.h"
#endif
#include "utils.h"
#include <cmath>
#include <cstdio>

#ifndef SIMULATION
#include "pico/stdlib.h"
#endif


#ifndef SIMULATION
void __not_in_flash_func(fft_filter::filter_block)(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture_i[], int16_t capture_q[]) {
#else
void fft_filter::filter_block(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture_i[], int16_t capture_q[]) {
#endif

#ifdef PICORX_FFT_FILTER_USE_CMSIS
  static q15_t tmp_fft_buf[2 * fft_size];
  static q15_t magnitudes[new_fft_size];

  // window
  arm_mult_q15(sample_real, window, sample_real, fft_size);
  arm_mult_q15(sample_imag, window, sample_imag, fft_size);

  for (uint16_t i = 0; i < fft_size; i++)
  {
    tmp_fft_buf[2 * i] = 512 * sample_real[i];
    tmp_fft_buf[(2 * i) + 1] = 512 * sample_imag[i];
  }

  // forward FFT
  arm_cfft_q15(&fft_i, tmp_fft_buf, 0, 1);
  arm_cmplx_mag_fast_q15(tmp_fft_buf, magnitudes, new_fft_size);

  for (uint16_t i = 0; i < fft_size; i++)
  {
    sample_real[i] = tmp_fft_buf[2 * i];
    sample_imag[i] = tmp_fft_buf[(2 * i) + 1];
  }
#else
  // window
  for (uint16_t i = 0; i < fft_size; i++) {
    sample_real[i] = product(sample_real[i], window[i]);
    sample_imag[i] = product(sample_imag[i], window[i]);
  }

  // forward FFT
  fixed_fft(sample_real, sample_imag, 8);
#endif

  if(filter_control.capture)
  {
    for (uint16_t i = 0; i < fft_size; i++) {
      capture_i[i] = sample_real[i];
      capture_q[i] = sample_imag[i];
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
      sample_real[i] = sample_real[i];
      sample_imag[i] = sample_imag[i];

      //capture highest and second highest peak
#ifdef PICORX_FFT_FILTER_USE_CMSIS
      const uint16_t magnitude = magnitudes[i];
#else
      const uint16_t magnitude = rectangular_2_magnitude(sample_real[i], sample_imag[i]);
#endif
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
      sample_real[new_idx] = sample_real[fft_size - (new_fft_size/2u) + i + 1];
      sample_imag[new_idx] = sample_imag[fft_size - (new_fft_size/2u) + i + 1];

      //capture highest and second highest peak
#ifdef PICORX_FFT_FILTER_USE_CMSIS
      const uint16_t magnitude = magnitudes[new_idx];
#else
      const uint16_t magnitude = rectangular_2_magnitude(sample_real[new_idx], sample_imag[new_idx]);
#endif
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



  if(filter_control.enable_auto_notch)
  {
    //check for a consistent
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

#ifdef PICORX_FFT_FILTER_USE_CMSIS
  for (uint16_t i = 0; i < new_fft_size; i++)
  {
    tmp_fft_buf[2 * i] = sample_real[i];
    tmp_fft_buf[2 * i + 1] = sample_imag[i];
  }

  // inverse FFT
  arm_cfft_q15(&ifft_i, tmp_fft_buf, 1, 1);

  for (uint16_t i = 0; i < new_fft_size; i++)
  {
    sample_real[i] = tmp_fft_buf[2 * i] / 2;
    sample_imag[i] = tmp_fft_buf[(2 * i) + 1] / 2;
  }
#else
  // inverse FFT
  fixed_ifft(sample_real, sample_imag, 7);
#endif

}


#ifndef SIMULATION
void __not_in_flash_func(fft_filter::process_sample)(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture_i[], int16_t capture_q[]) {
#else
void fft_filter::process_sample(int16_t sample_real[], int16_t sample_imag[], s_filter_control &filter_control, int16_t capture_i[], int16_t capture_q[]) {
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
  filter_block(real, imag, filter_control, capture_i, capture_q);

  for (uint16_t i = 0; i < (new_fft_size/2u); i++) {
    sample_real[i] = real[i] + last_output_real[i];
    sample_imag[i] = imag[i] + last_output_imag[i];
    last_output_real[i] = real[new_fft_size/2u + i];
    last_output_imag[i] = imag[new_fft_size/2u + i];
  }

}
