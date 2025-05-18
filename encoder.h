#pragma once

#include <cstdint>
#include "settings.h"

class encoder {
 public:
  virtual int32_t get_change(void) = 0;

  int32_t control(int32_t &value, int32_t min, int32_t max) {
    int32_t position_change = get_change();
    value += position_change;
    if (value > max) value = min;
    if (value < min) value = max;
    return position_change;
  }

 protected:
  encoder(s_global_settings &settings) : settings(settings) {};
  int32_t new_position = 0;
  int32_t old_position = 0;

  s_global_settings &settings;
};
