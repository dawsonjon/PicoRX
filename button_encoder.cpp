#include "button_encoder.h"

#include <algorithm>

//#include "ui.h"
#include "pins.h"

static const uint32_t LEVELS = 6;
static const uint32_t TRESHOLD_US = 1000000;
static const uint32_t rates[LEVELS] = {200000, 100000, 50000,
                                       20000,  20000,  20000};
static const uint8_t increments[LEVELS] = {1, 1, 1, 1, 2, 4};

button_encoder::button_encoder(s_global_settings &settings)
    : encoder(settings) {
  gpio_init(PIN_AB);
  gpio_set_dir(PIN_AB, GPIO_IN);
  gpio_pull_up(PIN_AB);
  gpio_init(PIN_B);
  gpio_set_dir(PIN_B, GPIO_IN);
  gpio_pull_up(PIN_B);
}

int32_t button_encoder::get_change(void) {
  int32_t delta = new_position - old_position;
  old_position = new_position;
  if (settings.reverse_encoder) {
    return -delta;
  } else {
    return delta;
  }
}

void button_encoder::update(void) {
  const uint32_t now = time_us_32();

  switch (state) {
    case idle:
      if (!gpio_get(PIN_AB)) {
        start_time = now;
        update_time = now;
        state = right;
        new_position++;
      } else if (!gpio_get(PIN_B)) {
        start_time = now;
        update_time = now;
        state = left;
        new_position--;
      }
      break;

    case right:
      if (gpio_get(PIN_AB)) {
        state = idle;
      } else {
        const uint32_t level =
            std::min((now - start_time) / TRESHOLD_US, LEVELS - 1);

        if ((now - update_time) > rates[level]) {
          new_position += increments[level];
          update_time = now;
        }
      }
      break;

    case left:
      if (gpio_get(PIN_B)) {
        state = idle;
      } else {
        const uint32_t level =
            std::min((now - start_time) / TRESHOLD_US, LEVELS - 1);

        if ((now - update_time) > rates[level]) {
          new_position -= increments[level];
          update_time = now;
        }
      }
      break;
  }
}
