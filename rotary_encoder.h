#pragma once

#include "encoder.h"
#include "hardware/pio.h"

class rotary_encoder : public encoder {
 public:
  rotary_encoder(const uint32_t (&settings)[16]);
  int32_t get_change(void);

 private:
  const uint32_t sm = 0;
  const PIO pio = pio1;
};
