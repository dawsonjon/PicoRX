#pragma once

#include "encoder.h"
#include "hardware/pio.h"

class button_encoder : public encoder {
 public:
  button_encoder(s_global_settings settings);
  int32_t get_change(void);
  void update(void);

 private:
  enum e_button_encoder_state { idle, right, left };
  e_button_encoder_state state = idle;
  uint32_t start_time = 0;
  uint32_t update_time = 0;
};
