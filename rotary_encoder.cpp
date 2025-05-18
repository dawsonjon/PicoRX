#include "rotary_encoder.h"

#include "quadrature_encoder.pio.h"
#include "ui.h"
#include "pins.h"


rotary_encoder::rotary_encoder(s_global_settings settings)
    : encoder(settings) {
  gpio_set_function(PIN_AB, GPIO_FUNC_PIO1);
  gpio_set_function(PIN_AB + 1, GPIO_FUNC_PIO1);
  uint offset = pio_add_program(pio, &quadrature_encoder_program);
  quadrature_encoder_program_init(pio, sm, offset, PIN_AB, 1000);
  if (settings.encoder_resolution & 1) {
    new_position = -((quadrature_encoder_get_count(pio, sm) + 1) / 2);
  } else {
    new_position = -((quadrature_encoder_get_count(pio, sm) + 2) / 4);
  }
  old_position = new_position;
}

int32_t rotary_encoder::get_change(void) {
  if (settings.encoder_resolution & 1) {
    new_position = -((quadrature_encoder_get_count(pio, sm) + 1) / 2);
  } else {
    new_position = -((quadrature_encoder_get_count(pio, sm) + 2) / 4);
  }
  int32_t delta = new_position - old_position;
  old_position = new_position;
  if (settings.reverse_encoder & 1) {
    return -delta;
  } else {
    return delta;
  }
}
