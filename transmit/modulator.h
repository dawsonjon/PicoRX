//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: modulator.h
// description:
// License: MIT
//

#ifndef __MODULATOR_H__
#define __MODULATOR_H__

#include "half_band_filter.h"
#include "half_band_filter2.h"
#include "../src/rx_definitions.h"

struct s_debug{
  int16_t filtered_audio;
  int16_t raw_i;
  int16_t raw_q;
  int16_t clipped_i;
  int16_t clipped_q;
  uint16_t env;
};

class modulator
{

  private:
  int16_t last_phase = 0;
  uint8_t ssb_phase = 0;
  half_band_filter2 ssb_filter;
  half_band_filter2 ssb_filter2;
  half_band_filter2 audio_filter2;
  half_band_filter audio_filter;

  public:
  modulator();
  void process_sample(uint8_t mode, int16_t audio, int16_t &i, int16_t &q,  uint16_t &magnitude, int16_t &phase, uint32_t fm_deviation_f15, s_debug &debug);
  void process_envelope(int16_t &i, int16_t &q, s_debug &debug);
};

#endif
