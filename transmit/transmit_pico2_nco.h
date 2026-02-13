//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: transmit_nco.h
// description: PIO based NCO for Pi Pico
// License: MIT
//

#ifndef NCO_H__
#define NCO_H__

#include "hardware/dma.h"
#include "hardware/sync.h"
#include "stream_bits.pio.h"
#include <cmath>

static const uint8_t bits_per_word = 32u;
static const uint8_t max_waveforms_per_sample = 200u;
static const uint16_t waveform_length_bits = 512u;
static const uint16_t waveform_length_words = waveform_length_bits / bits_per_word;

class transmit_nco {
private:
  uint8_t m_rf_pin;
  bool m_dither;
  uint8_t m_phase_dither;
  uint8_t m_waveform_phase_dither;

  uint32_t nco_dma, chain_dma, sm;
  dma_channel_config nco_dma_cfg;
  dma_channel_config chain_dma_cfg;
  int64_t index_f32 = 0u;
  uint8_t ping_pong = 0u;
  uint64_t interrupts;
  bool dma_started = false;
  static const uint32_t fraction_bits = 32u;
  int64_t index_increment_f32;
  int64_t wrap_f32;
  int64_t phase_step_clocks_f32;
  uint offset;

  void initialise_waveform_buffer(uint32_t buffer[],
                                  uint32_t waveform_length_words,
                                  double normalised_frequency);

public:
  transmit_nco(const uint8_t rf_pin, double clock_frequency_Hz, double frequency_Hz, uint8_t waveform_phase_dither, uint8_t phase_dither);
  ~transmit_nco();
  double get_sample_frequency_Hz(double clock_frequency_Hz, uint8_t waveforms_per_sample);
  uint8_t get_waveforms_per_sample(double clock_frequency_Hz, double sample_frequency_Hz);
  void output_sample(int16_t phase, uint8_t waveforms_per_sample);
};

#endif
