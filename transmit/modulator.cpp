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

#ifndef SIMULATION
#include "pico/stdlib.h"
#endif

#include "../src/utils.h"
#include "cordic.h"
#include "modulator.h"

#include <cstdio>

modulator ::modulator() { cordic_init(); }


#ifndef SIMULATION
void __not_in_flash_func(modulator::process_envelope)(int16_t &i, int16_t &q, s_debug &debug) {
#else
void modulator ::process_envelope(int16_t &i, int16_t &q, s_debug &debug) {
#endif

  //uint16_t env = rectangular_2_magnitude(i, q);
  uint16_t env = sqrt(i*i+q*q);
  debug.env = env;

  uint16_t limit = 5910; //32767/(2*filter_gain*filter_gain)

  if(env > limit) {
    uint16_t overshoot;
    overshoot = env-limit;
    overshoot = overshoot * 3;
    overshoot += limit;
    i = (int32_t)i*limit/overshoot;
    q = (int32_t)q*limit/overshoot;
  }
  i = (int32_t)i*32767/limit;
  q = (int32_t)q*32767/limit;

}

#ifndef SIMULATION
void __not_in_flash_func(modulator ::process_sample)(uint8_t mode, int16_t audio, int16_t &i,
                                int16_t &q, uint16_t &magnitude, int16_t &phase,
                                uint32_t fm_deviation_f15, s_debug &debug) {
#else
void modulator ::process_sample(uint8_t mode, int16_t audio, int16_t &i,
                                int16_t &q, uint16_t &magnitude, int16_t &phase,
                                uint32_t fm_deviation_f15, s_debug &debug) {
#endif

  if(mode != CW)
  {
    audio_filter.filter(audio);
    debug.filtered_audio = audio;
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
    debug.raw_i = ii;
    debug.raw_q = qq;
    process_envelope(ii, qq, debug);
    debug.clipped_i = ii;
    debug.clipped_q = qq;
    ssb_filter2.filter(ii, qq);

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
    i = sample_i[ssb_phase];
    q = sample_q[ssb_phase];
    audio_filter2.filter(i, q);
    i = (int32_t)i*2798 >> 10; // gain = 1.67 0.98 * 1024/(gain * gain)
    q = (int32_t)q*2798 >> 10;

    //static int16_t i_prev=0, q_prev=0;
    //ii = (int32_t(i)*i_prev + int32_t(q)*q_prev)>>15;
    //qq = (int32_t(q)*i_prev - int32_t(i)*q_prev)>>15;
    //i_prev = i;
    //q_prev = q;

    //int16_t dphase=0;
    //static int16_t phase_acc=0;
    //cordic_rectangular_to_polar(ii, qq, magnitude, dphase);
    //if(dphase > 16384) dphase = 16384;
    //if(dphase < -16384) dphase = -16384;
    //uint32_t gain = (magnitude << 15) / (magnitude + 1000);
    //dphase = (int32_t)dphase * gain >> 15;
    //printf("i %i q %i ii %i qq %i magnitude %u dphase %i gain %u\n", i, q, ii, qq, magnitude, dphase, magnitude+100);

    //phase_acc+=dphase;
    //phase = phase_acc;
    cordic_rectangular_to_polar(i, q, magnitude, phase);
    //static int16_t prev_phase = 0;
    //int16_t dphase = phase - prev_phase;

    //if(magnitude < 100) {
       //phase = prev_phase;
    //} else {
       //prev_phase = phase;
    //}

    magnitude = magnitude > 32767 ? 32767 : magnitude;
    magnitude <<= 1;
  }
}
