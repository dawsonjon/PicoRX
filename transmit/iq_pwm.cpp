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

#include "iq_pwm.h"
#include "stdio.h"

iq_pwm::iq_pwm(const uint8_t i_pin, const uint8_t q_pin) {
  m_i_pin = i_pin;
  m_q_pin = q_pin;
  gpio_set_function(i_pin, GPIO_FUNC_PWM);
  gpio_set_function(q_pin, GPIO_FUNC_PWM);
  gpio_set_drive_strength(i_pin, GPIO_DRIVE_STRENGTH_12MA);
  gpio_set_drive_strength(q_pin, GPIO_DRIVE_STRENGTH_12MA);
  const uint16_t pwm_max = 254; // 8 bit pwm
  const int i_pwm_slice = pwm_gpio_to_slice_num(i_pin); // GPIO10
  const int q_pwm_slice = pwm_gpio_to_slice_num(q_pin); // GPIO12
  pwm_config i_config = pwm_get_default_config();
  pwm_config q_config = pwm_get_default_config();

  pwm_config_set_clkdiv(&i_config, 2.f); // 125MHz
  pwm_config_set_wrap(&i_config, pwm_max);
  pwm_config_set_output_polarity(&i_config, false, false);
  pwm_init(i_pwm_slice, &i_config, true);

  pwm_config_set_clkdiv(&q_config, 2.f); // 125MHz
  pwm_config_set_wrap(&q_config, pwm_max);
  pwm_config_set_output_polarity(&q_config, false, false);
  pwm_init(q_pwm_slice, &q_config, true);
}

iq_pwm::~iq_pwm() {
  // disable GPIO, pullup/pulldown resistors should be installed
  // to switch off transistors when pin is high impedance
  gpio_deinit(m_i_pin);
  gpio_deinit(m_q_pin);
}

void __not_in_flash_func(iq_pwm::output_sample)(int16_t i, int16_t q) {
    i = (i + 32768) >> 8;
    q = (q + 32768) >> 8;
    pwm_set_gpio_level(m_i_pin, i);
    pwm_set_gpio_level(m_q_pin, q);
}
