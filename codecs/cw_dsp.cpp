//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: cw_dsp.cpp
// description: Multi-Channel FFT-based DSP functionality for CW decoder
// License: MIT
//

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "cw_dsp.h"
#include "cw_fft.h"
#include "cw_utils.h"
#include "../src/utils.h"
#include <algorithm>

// #define LOGGING

#ifdef LOGGING
#ifdef ARDUINO
#include <Arduino.h>
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#include <cstdio>
#define DEBUG_PRINTF(...) std::printf(__VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(...)
#endif

// Function to generate a window
void generate_window(int32_t* window, int size)
{
  for (uint16_t i = 0; i < size; i++) {
    float multiplier = 0.5 * (1 - cosf(2 * M_PI * i / size));
    window[i] = multiplier * 65536;
  }
}

// apply a flat-topped window to reduce scalloping loss
void apply_window(int16_t i[], int16_t q[], int32_t window[], const uint16_t n)
{
  for (uint16_t idx = 0; idx < n; ++idx) {
    assert(idx >= 0);
    assert(idx < FRAME_SIZE);
    int32_t i_val = i[idx];
    int32_t q_val = q[idx];
    i_val = window[idx] * i_val / 65536;
    q_val = window[idx] * q_val / 65536;
    i[idx] = i_val;
    q[idx] = q_val;
  }
}

void c_cw_dsp ::decode(uint16_t channel, std::string text, std::string partial)
{
  (void) channel;
  (void) text;
  (void) partial;
  DEBUG_PRINTF("decode on channel %u %s\n", channel, text.c_str());
}

static void max_magnitude(uint32_t magnitude[], uint32_t threshold[], uint8_t start_bin,
                          uint8_t stop_bin, uint16_t& max_bin, uint32_t& max,
                          uint32_t& max_threshold)
{
  max = 0;
  max_bin = 0;
  max_threshold = 0;
  for (uint16_t idx = start_bin; idx < stop_bin; ++idx) {
    assert(idx >= 0);
    assert(idx < FRAME_SIZE / 2);
    if (magnitude[idx] > max) {
      max_bin = idx;
      max = magnitude[idx];
    }
    if (threshold[idx] > max_threshold) {
      max_threshold = threshold[idx];
    }
  }
}

void c_cw_dsp ::flush()
{
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    print_element("decode_bins", channel * CHANNEL_SIZE);
    channels[channel].decoder.decode(channels[channel].observations,
                                     channels[channel].num_observations);
    decode(channel, channels[channel].decoder.get_text(),
           channels[channel].decoder.get_text_partial());
    channels[channel].num_observations = 0;
    channels[channel].duration = 0;
  }
}

void c_cw_dsp ::process_channels()
{

  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {

    uint8_t start_bin = channel * CHANNEL_SIZE;
    uint8_t stop_bin = start_bin + CHANNEL_SIZE - 1;

    uint16_t max_bin;
    uint32_t max;
    uint32_t max_threshold;
    max_magnitude(magnitude, threshold, start_bin, stop_bin, max_bin, max, max_threshold);
    DEBUG_PRINTF("channel: %u magnitude: %u threshold: %u noise: %f snr: %f\n value %u", channel,
                 magnitude[max_bin], max_threshold, noise_estimate[max_bin], get_snr(channel),
                 channels[channel].value);

    // measure signal present periods
    bool value;
    if (channels[channel].value) {
      channels[channel].snr =
          (0.99 * channels[channel].snr) + (0.01 * max / noise_estimate[max_bin]);
      value = max > (0.7 * max_threshold);
    } else {
      channels[channel].snr =
          (0.999 * channels[channel].snr) + (0.001 * max / noise_estimate[max_bin]);
      value = max > max_threshold;
    }
    channels[channel].duration++;
    if (value != channels[channel].value) {
      s_observation observation = {channels[channel].value, FRAME_MS * channels[channel].duration};
      channels[channel].duration = 0;
      channels[channel].value = value;
      channels[channel].observations[channels[channel].num_observations++] = observation;
    }

    // timeout
    if (channels[channel].duration == TIMEOUT) {
      if (get_snr(channel) > 9.0f) {
        print_element("decode_bins", channel * CHANNEL_SIZE);

        channels[channel].decoder.decode(channels[channel].observations,
                                         channels[channel].num_observations);

        decode(channel, channels[channel].decoder.get_text(),
               channels[channel].decoder.get_text_partial());
      }
      channels[channel].num_observations = 0;
      channels[channel].duration = 0;
      channels[channel].trained = false; // treat this as the end of a communication and retrain
      channels[channel].snr = 0;
      channels[channel].decoder.reset();
    }

    // full buffer
    if (channels[channel].num_observations == OBSERVATION_BUFFER_SIZE ||
        (channels[channel].trained &&
         channels[channel].num_observations == OBSERVATION_BURST_SIZE)) {

      // check for feasible SNR to perform decode
      if (get_snr(channel) > 9.0f) {

        channels[channel].decoder.decode(channels[channel].observations,
                                         channels[channel].num_observations);

        decode(channel, channels[channel].decoder.get_text(),
               channels[channel].decoder.get_text_partial());

        channels[channel].trained = true;
      }

      channels[channel].num_observations = 0;
      channels[channel].duration = 0;
    }
  }
}

void c_cw_dsp ::process_frame()
{
  print_frame("magnitudes", magnitude);

  static bool initialised = false;
  static uint16_t gate_count[FRAME_SIZE / 2];
  if (!initialised) {
    for (uint16_t idx = 0; idx < FRAME_SIZE / 2; ++idx) {
      noise_estimate[idx] = magnitude[idx];
      gate_count[idx] = 0;
    }
    initialised = true;
  }

  for (uint16_t idx = 0; idx < FRAME_SIZE / 2; ++idx) {
    if ((magnitude[idx] < 2.0 * noise_estimate[idx]) || magnitude[idx] < 5) {
      noise_estimate[idx] = (0.99 * noise_estimate[idx]) + (0.01 * magnitude[idx]);
      gate_count[idx] = 0;
    } else if (gate_count[idx] > 50) {
      noise_estimate[idx] = (0.9 * noise_estimate[idx]) + (0.1 * magnitude[idx]);
    } else {
      gate_count[idx]++;
    }
    noise_estimate[idx] = std::max(noise_estimate[idx], 1.0f);
    threshold[idx] = noise_estimate[idx] * thresh_mult;
  }

  print_frame_float("noise_estimate", noise_estimate);
  print_frame("threshold", threshold);

  // process active signals
  process_channels();
  frame_count++;
}

c_cw_dsp ::c_cw_dsp()
    : channels{s_channel(0), s_channel(1), s_channel(2), s_channel(3),
               s_channel(4), s_channel(5), s_channel(6)}
{
  // clear buffer
  for (uint16_t idx = 0; idx < FRAME_SIZE; idx++) {
    i[idx] = 0;
    q[idx] = 0;
  }
  // initialise channels
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
    channels[channel].duration = 0;
    channels[channel].snr = 0.0f;
    channels[channel].value = false;
    channels[channel].observations[channels[channel].num_observations++];
    channels[channel].num_observations = 0;
    channels[channel].trained = false;
  }
  generate_window(window, FRAME_SIZE);
  for (uint16_t idx = 0; idx < FRAME_SIZE / 2; idx++) {
    threshold[idx] = 0;
    noise_estimate[idx] = 0;
  }
  frame_count = 0;
}

uint32_t c_cw_dsp ::get_buffer_percent(int channel)
{
  uint32_t percentage = 100 * channels[channel].num_observations / OBSERVATION_BUFFER_SIZE;
  if (channels[channel].trained)
    return 100;
  return percentage;
}

float c_cw_dsp ::get_snr(int channel)
{
  if (channels[channel].snr > 0.0001f)
    //-6.3 converts from 117 to 500Hz bandwidth
    return 20.0f * log10(channels[channel].snr) - 6.3f;
  else
    return -99.0f;
}

void c_cw_dsp ::process_sample(int16_t sample)
{
  // resample from 15000Hz to 7500 Hz
  uint32_t sample_ratio_f16 = 7500 * 65536 / 15000;
  sample_number_f16 += sample_ratio_f16;
  if (sample_number_f16 < 65536)
    return;
  sample_number_f16 -= 65536;
  i[frequency_count++] = sample;

  // sample rate is 15000, 1st Nyquist -3750 to +3750, but only 0 to ~2500Hz is used.
  // at 7500Hz, each bin in a 64 point FFT represents 117Hz
  if (frequency_count == FRAME_SIZE) {
    frequency_count = 0;

    apply_window(i, q, window, FRAME_SIZE);
    cw_fixed_fft(i, q, 6);

    for (uint16_t idx = 0; idx < FRAME_SIZE / 2; idx++) {
      magnitude[idx] = rectangular_2_magnitude(i[idx], q[idx]);
    }
    memcpy(magnitude_copy, magnitude, sizeof(magnitude));
    process_frame();
  }
}

//********** debug code from here down **********

void c_cw_dsp ::print_frame(const char filename[], uint32_t frame[])
{
  (void)filename;
  (void)frame;
#ifdef LOG_TO_FILE
  FILE* outf = fopen(filename, "a");
  for (uint16_t f = 0; f < FRAME_SIZE / 2; f++) {
    fprintf(outf, "%i ", (int)frame[f]);
    fprintf(outf, " ");
  }
  fprintf(outf, "\n");
  fclose(outf);
#endif
}

void c_cw_dsp ::print_frame_float(const char filename[], float frame[])
{
  (void)filename;
  (void)frame;
#ifdef LOG_TO_FILE
  FILE* outf = fopen(filename, "a");
  for (uint16_t f = 0; f < FRAME_SIZE / 2; f++) {
    fprintf(outf, "%f ", (float)frame[f]);
    fprintf(outf, " ");
  }
  fprintf(outf, "\n");
  fclose(outf);
#endif
}

void c_cw_dsp ::print_element(const char filename[], uint32_t element)
{
  (void)filename;
  (void)element;
#ifdef LOG_TO_FILE
  FILE* outf = fopen(filename, "a");
  fprintf(outf, "%i", (int)element);
  fprintf(outf, "\n");
  fclose(outf);
#endif
}

void c_cw_dsp ::print_bool(const char filename[], bool element)
{
  (void)filename;
  (void)element;
#ifdef LOG_TO_FILE
  FILE* outf = fopen(filename, "a");
  fprintf(outf, "%i", (int)element);
  fprintf(outf, "\n");
  fclose(outf);
#endif
}
