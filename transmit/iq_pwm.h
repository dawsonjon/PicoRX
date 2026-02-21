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
#include "pico/stdlib.h"
#include <cmath>

class iq_pwm {
private:
  uint8_t m_q_pin;
  uint8_t m_i_pin;

public:
  iq_pwm(const uint8_t i_pin, const uint8_t q_pin);
  ~iq_pwm();
  void output_sample(int16_t i, int16_t q);
};

#endif
