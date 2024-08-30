//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2024
// filename: battery_check.cpp
// description: Simple Battery Check Utility
// License: MIT
//

#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <stdio.h>

float read_adc_volts(uint8_t channel) {
  adc_select_input(channel);
  return 3.3f * adc_read() / 4096.0f;
}

int main() {
  stdio_init_all();

  // configure ADC
  uint8_t batt_adc = 3;
  uint8_t temp_adc = 4;
  adc_init();
  adc_gpio_init(29);//Battery - configure pin for ADC use
  adc_set_temp_sensor_enabled(true);
  adc_set_clkdiv(0); //flat out

  float batt_adc_volts = 0.0f;
  float temp_adc_volts = 0.0f;

  while (1) {
    batt_adc_volts = (0.9 * batt_adc_volts) + (0.1 * read_adc_volts(batt_adc));
    const float batt_volts = 3.0 * batt_adc_volts;

    temp_adc_volts = (0.9 * temp_adc_volts) + (0.1 * read_adc_volts(temp_adc));
    const float temp = 27.0f - (temp_adc_volts - 0.706f)/0.001721f;

    printf("Battery Voltage %5.3f temperature %5.3f degrees C\n", batt_volts, temp);
  }
}
