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

#ifndef ADC_H__
#define ADC_H__

#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"


class adc {
private:
  int32_t dc = 0;

public:
  adc(const uint8_t mic_pin, const uint8_t adc_input);
  int16_t get_sample();
};

#endif
