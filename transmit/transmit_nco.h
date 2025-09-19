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
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "stream_bits.pio.h"
#include <cmath>

class SinLut
{
  private:
    SinLut()
    {
      for (uint16_t i = 0; i < 2048; ++i) {
        waveform[i] = sinf(2.0 * M_PI * i/2048.0)*16383.0;
      }
    }

  public:
    static SinLut& instance()
    {
      static SinLut INSTANCE;
      return INSTANCE;
    }
    int16_t waveform[2048]; //11 bit lookup
};


class transmit_nco {
private:
  uint8_t m_rf_pin;
  bool m_dither;
  PIO pio = pio0;
  uint32_t nco_dma, chain_dma, sm;
  dma_channel_config nco_dma_cfg;
  dma_channel_config chain_dma_cfg;
  static const uint8_t max_waveforms_per_sample = 100u;
  int32_t index_f24 = 0u;
  uint8_t ping_pong = 0u;
  uint32_t interrupts;
  bool dma_started = false;
  static const uint8_t bits_per_word = 32u;
  static const uint16_t waveform_length_bits = 256u;
  static const uint16_t waveform_length_words =
      waveform_length_bits / bits_per_word;
  static const uint32_t fraction_bits = 24u;
  int32_t index_increment_f24;
  int32_t wrap_f24;
  int32_t phase_step_clocks_f24;
  uint offset;
  const uint32_t *buffer_addresses[2][max_waveforms_per_sample + 1];
  uint32_t buffer[bits_per_word * waveform_length_words * 2]
      __attribute__((aligned(4)));

  void initialise_waveform_buffer(uint32_t buffer[],
                                  uint32_t waveform_length_words,
                                  double normalised_frequency);

public:
  transmit_nco(const uint8_t rf_pin, double clock_frequency_Hz, double frequency_Hz, bool dither=false);
  ~transmit_nco();
  double get_sample_frequency_Hz(double clock_frequency_Hz, uint8_t waveforms_per_sample);
  uint8_t get_waveforms_per_sample(double clock_frequency_Hz, double sample_frequency_Hz);
  void output_sample(int16_t phase, uint8_t waveforms_per_sample);
};

#endif
