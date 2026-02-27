#pragma once

#include "encoder.h"

class rotary_encoder : public encoder {
 public:
  rotary_encoder(s_global_settings& _settings);
  int32_t get_change(void);
};
