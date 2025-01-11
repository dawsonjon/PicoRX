//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: adc.cpp
// description: ADC Interface Ham Transmitter
// License: MIT
//

#include "adc.h"

adc::adc(const uint8_t mic_pin, const uint8_t adc_input) {
  // configure ADC
  adc_init();
  adc_gpio_init(mic_pin);
  adc_select_input(adc_input);
}

int16_t __not_in_flash_func(adc::get_sample)() {
  int32_t adc_audio = adc_read();
  dc = dc + ((adc_audio - dc) >> 1);
  return (adc_audio - dc);
}
