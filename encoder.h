#pragma once

#include <cstdint>

class encoder {
 public:
  virtual int32_t get_change(void) = 0;

  int32_t control(int32_t *value, int32_t min, int32_t max) {
    int32_t position_change = get_change();
    *value += position_change;
    if (*value > max) *value = min;
    if (*value < min) *value = max;
    return position_change;
  }

 protected:
  encoder(const uint32_t (&settings)[16]) : settings(settings) {};
  int32_t new_position = 0;
  int32_t old_position = 0;

  const uint32_t initial_settings[16] = {0};
  const uint32_t (&settings)[16] = initial_settings;
};
