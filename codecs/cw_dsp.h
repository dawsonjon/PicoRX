//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: cw_dsp.h
// description: FFT-based Multi Channel CW Decoder DSP function.
// License: MIT
//

#ifndef _CW_DECODER_H__
#define _CW_DECODER_H__

#include <cstdint>
#include <list>
#include <vector>

#include "cw_decode.h"
#include "cw_dsp.h"

static const uint16_t FRAME_SIZE = 64;
// enough observations to train the decoder
static const uint16_t OBSERVATION_BUFFER_SIZE = 50;
// enough observations to update the decoder
static const uint16_t OBSERVATION_BURST_SIZE = 10;
static const uint16_t TIMEOUT = 500;
static const uint16_t NUM_CHANNELS = 6;
static const uint16_t CHANNEL_SIZE = 5;
static const float SAMPLE_FREQUENCY = 15000.0f;
static const float FRAME_MS = 1000.0f * 128.0f / SAMPLE_FREQUENCY;

struct s_channel
{
  uint16_t duration;
  bool value;
  s_observation observations[OBSERVATION_BUFFER_SIZE];
  uint16_t num_observations;
  c_cw_decoder decoder;
  bool trained;
  float snr;
  s_channel(int channel_number) : decoder(channel_number) {}
};

class c_cw_dsp
{

  uint16_t frequency_count = 0;
  uint32_t sample_number_f16 = 0;
  int16_t i[FRAME_SIZE];
  int16_t q[FRAME_SIZE];
  int32_t window[FRAME_SIZE];
  uint32_t magnitude[FRAME_SIZE / 2];
  uint32_t magnitude_copy[FRAME_SIZE / 2];
  uint32_t old_magnitude[FRAME_SIZE / 2];
  uint32_t new_magnitude[FRAME_SIZE / 2];
  uint32_t threshold[FRAME_SIZE / 2];
  float noise_estimate[FRAME_SIZE / 2];
  uint32_t smoothed_magnitude[FRAME_SIZE / 2];
  uint32_t frame_count;
  float thresh_mult = 6;

  s_channel channels[7];

  void print_frame(const char filename[], uint32_t frame[]);
  void print_frame_float(const char filename[], float frame[]);
  void print_element(const char filename[], uint32_t element);
  void print_bool(const char filename[], bool element);
  void process_frame();
  void process_channels();
  virtual void decode(uint16_t cluster, std::string text, std::string partial);

public:
  c_cw_dsp();
  void process_sample(int16_t sample);
  void flush();
  float get_WPM(int channel)
  {
    if (channel >= NUM_CHANNELS || !channels[channel].trained)
      return 0.0f;
    return channels[channel].decoder.get_WPM();
  }
  uint32_t* get_magnitudes() { return &magnitude_copy[0]; }

  uint32_t get_buffer_percent(int channel);
  float get_snr(int channel);
  void set_threshold(float _thresh_mult) { thresh_mult = _thresh_mult; }
};

#endif
