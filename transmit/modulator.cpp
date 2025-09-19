//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: modulator.cpp
// description:
// License: MIT
//

#include <cmath>
#include "pico/stdlib.h"

#include "codecs/cordic.h"
#include "modulator.h"

modulator ::modulator() { cordic_init(); }

void __not_in_flash_func(modulator ::process_sample)(uint8_t mode, int16_t audio, int16_t &i,
                                int16_t &q, uint16_t &magnitude, int16_t &phase,
                                uint32_t fm_deviation_f15) {

  if(mode != CW)
  {
    audio = (int32_t)audio * 65200 >> 16;
    audio_filter.filter(audio);
  }

  if (mode == AM || mode == AMSYNC) {
    magnitude = audio + 32767;
    phase = 0;
  } else if (mode == CW) {
    magnitude = audio * 2;
    phase = 0;
  } else if (mode == FM) {
    magnitude = 65535;
    phase = last_phase + ((audio * fm_deviation_f15) >> 15);
    last_phase = phase;
  } else if (mode == LSB || mode == USB) {
    // shift frequency by +FS/4
    //       __|__
    //   ___/  |  \___
    //         |
    //   <-----+----->

    //        | ____
    //  ______|/    \.
      //        |
    //  <-----+----->

    // filter -Fs/4 to +Fs/4

    //        | __
    //  ______|/  \__
    //        |
    //  <-----+----->

    if (mode == LSB) {
      ssb_phase = (ssb_phase + 1) & 3u;
    } else {
      ssb_phase = (ssb_phase - 1) & 3u;
    }

    const int16_t audio_i[4] = {audio, 0, (int16_t)(-audio), 0};
    const int16_t audio_q[4] = {0, (int16_t)(-audio), 0, audio};
    int16_t ii = audio_i[ssb_phase];
    int16_t qq = audio_q[ssb_phase];
    ssb_filter.filter(ii, qq);

    // shift frequency by -FS/4
    //         | __
    //   ______|/  \__
    //         |
    //   <-----+----->

    //     __ |
    //  __/  \|______
    //        |
    //  <-----+----->

    const int16_t sample_i[4] = {(int16_t)(-qq), (int16_t)(-ii), qq, ii};
    const int16_t sample_q[4] = {ii, (int16_t)(-qq), (int16_t)(-ii), qq};
    i = sample_i[ssb_phase] << 1;
    q = sample_q[ssb_phase] << 1;

    cordic_rectangular_to_polar(i, q, magnitude, phase);
    magnitude = magnitude > 32767 ? 32767 : magnitude;
    magnitude <<= 1;
  }
}
