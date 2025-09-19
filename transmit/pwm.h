//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: pwm.cpp
// description: PWM Magnitude for Ham Transmitter
// License: MIT
//

#ifndef PWM_H__
#define PWM_H__

#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <cmath>

class pwm {
private:
  uint8_t m_magnitude_pin;

public:
  pwm(const uint8_t magnitude_pin);
  ~pwm();
  void output_sample(uint16_t magnitude, const uint8_t pwm_min, const uint8_t pwm_max, const uint8_t pwm_threshold);
};

#endif
