//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: signal_generator.cpp
// description: Test signal generator for Ham Transmitter
// License: MIT
//

#include "signal_generator.h"
#include <cmath>

signal_generator::signal_generator() {
  // initialise lookup table
  for (uint16_t idx = 0; idx < 1024; idx++) {
    sin_table[idx] = 32767 * sin(2 * M_PI * idx / 1024);
  }
}

int16_t signal_generator::get_sample(uint32_t frequency_steps) {
  int16_t sample = sin_table[phase >> 22];
  phase += frequency_steps;
  return sample;
}

uint32_t signal_generator::frequency_steps(double normalized_frequency) {
  return pow(2.0, 32.0) * normalized_frequency;
}
