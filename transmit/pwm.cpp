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

#include "pwm.h"
#include "stdio.h"

pwm::pwm(const uint8_t magnitude_pin) {
  printf("PWM INIT==========\n");
  m_magnitude_pin = magnitude_pin;
  gpio_set_function(magnitude_pin, GPIO_FUNC_PWM);
  gpio_set_drive_strength(magnitude_pin, GPIO_DRIVE_STRENGTH_12MA);
  const uint16_t pwm_max = 254; // 8 bit pwm
  const int magnitude_pwm_slice = pwm_gpio_to_slice_num(magnitude_pin); // GPIO1
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 2.f); // 125MHz
  pwm_config_set_wrap(&config, pwm_max);
  pwm_config_set_output_polarity(&config, false, false);
  pwm_init(magnitude_pwm_slice, &config, true);
}

pwm::~pwm() {
  // disable GPIO, pullup/pulldown resistors should be installed
  // to switch off transistors when pin is high impedance
  gpio_deinit(m_magnitude_pin);
}

void __not_in_flash_func(pwm::output_sample)(uint16_t magnitude, uint8_t pwm_min, uint8_t pwm_max, uint8_t pwm_threshold) {
    magnitude >>= 8;

    //don't output anything unless magnitude exceeds threhold
    static uint8_t hang_time=0;
    if(magnitude > pwm_threshold)
    {
      hang_time = 255;
    }
    else if(hang_time)
    {
      hang_time--;
    }

    //scale PWM according to min/max values
    if(hang_time)
    {
      magnitude = magnitude * (pwm_max - pwm_min) / 255;
      magnitude += pwm_min;
    }
    else
    {
      magnitude = 0;
    }
    printf("MAGNITUDE===========%d\n", magnitude);
    pwm_set_gpio_level(m_magnitude_pin, magnitude);
}
