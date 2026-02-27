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

#include "../src/utils.h"
#include "cordic.h"
#include "modulator.h"

#include <cstdio>

modulator ::modulator() { cordic_init(); }

void __not_in_flash_func(modulator ::process_sample)(uint8_t mode, int16_t audio, int16_t &i,
                                int16_t &q, uint16_t &magnitude, int16_t &phase,
                                uint32_t fm_deviation_f15) {

  static uint32_t max_audio_magnitude = 0;
  //static uint32_t audio_gain = 0;
  if(mode != CW)
  {
    audio_filter.filter(audio);
    if(abs(audio) > max_audio_magnitude) {
      max_audio_magnitude = abs(audio);
      //audio_gain = (62260/max_audio_magnitude) << 8;
    }
    //audio = (int32_t)audio * audio_gain >> 8;
  }

  if (mode == AM || mode == AMSYNC) {
    magnitude = audio + 32767;
    phase = 0;
    i = (audio>>1) + 16383;
    q = 0;
  } else if (mode == CW) {
    magnitude = audio * 2;
    i = audio;
    q = 0;
    phase = 0;
  } else if (mode == FM) {
    magnitude = 65535;
    phase = last_phase + ((audio * fm_deviation_f15) >> 15);
    last_phase = phase;
    const int16_t scaled_phase = phase >> 5; //16 -> 11 bits
    i =  sin_table[(scaled_phase+512u) & 0x7ff];
    q = -sin_table[scaled_phase & 0x7ff];

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

    static uint32_t max_iq_magnitude = 0;
    static uint32_t iq_gain = 450; //1.5
    if(abs(i) > max_iq_magnitude) {
      max_iq_magnitude = abs(i);
      //iq_gain = (62260/max_iq_magnitude) << 8;
    }
    if(abs(q) > max_iq_magnitude) {
      max_iq_magnitude = abs(q);
      //iq_gain = (62260/max_iq_magnitude) << 8;
    }
    i = (int32_t)i * iq_gain >> 8;
    q = (int32_t)q * iq_gain >> 8;

    cordic_rectangular_to_polar(i, q, magnitude, phase);
    magnitude = magnitude > 32767 ? 32767 : magnitude;
    magnitude <<= 1;
    static uint32_t max_magnitude = 0;
    if(magnitude > max_magnitude) {
      max_magnitude = magnitude;
      //iq_gain = (62260/max_iq_magnitude) << 8;
    }
  }
}
