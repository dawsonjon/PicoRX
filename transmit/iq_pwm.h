//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: iq_pwm.cpp
// description: PWM iq for Ham Transmitter
// License: MIT
//

#ifndef IQ_PWM_H__
#define IQ_PWM_H__

#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"
#include <cmath>

class iq_pwm {
private:
  uint8_t m_q_pin;
  uint8_t m_i_pin;

  int i_dma;
  int q_dma;
  int dma_timer;
  dma_channel_config i_cfg;
  dma_channel_config q_cfg;
  int i_slice;
  int q_slice;

public:
  iq_pwm(const uint8_t i_pin,
         const uint8_t q_pin,
         const uint32_t audio_sample_rate,
         const uint32_t cpuFrequencyHz);

  ~iq_pwm();
  void output_sample(int16_t i, int16_t q);
};

#endif
