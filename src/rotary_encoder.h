#pragma once

#include "encoder.h"
#include "hardware/pio.h"

class rotary_encoder : public encoder {
 public:
  rotary_encoder(s_global_settings &settings);
  int32_t get_change(void);

 private:
  const uint32_t sm = 0;
  const PIO pio = pio1;
};
