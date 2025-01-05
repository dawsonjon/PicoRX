//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: signal_generator.h
// description: Test signal generator for Ham Transmitter
// License: MIT
//

#ifndef SIGNAL_GENERATOR_H__
#define SIGNAL_GENERATOR_H__
#include <cstdint>

static int16_t sin_table[1024];
class signal_generator {
private:
  uint32_t phase = 0;

public:
  signal_generator();
  int16_t get_sample(uint32_t frequency_steps);
  uint32_t frequency_steps(double normalized_frequency);
};

#endif
