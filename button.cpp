#include "button.h"

#include <cstdio>

#include "pico/stdlib.h"

button ::button(uint8_t gpio_num) : gpio_num(gpio_num) {
  button_state = idle;
  time_pressed = 0;
  pressed_count = 0;
  held_count = 0;

  gpio_init(gpio_num);
  gpio_set_dir(gpio_num, GPIO_IN);
  gpio_pull_up(gpio_num);
}

void button ::update_state() {
  if (button_state == idle) {
    if (!gpio_get(gpio_num))  // on
    {
      time_pressed = time_us_32();
      button_state = decoding;
    }
  } else if (button_state == decoding) {
    if (gpio_get(gpio_num))  // off
    {
      pressed_count++;
      button_state = idle;
    } else {  // on
      if ((time_us_32() - time_pressed) > (250UL * 1000)) {
        held_count++;
        button_state = active;
      }
    }
  } else if (button_state == active) {
    if (gpio_get(gpio_num))  // off
    {
      held_count = 0;
      button_state = idle;
    }
  }
}

bool button :: is_keyed()
{
  return !gpio_get(gpio_num);
}

bool button ::is_pressed() {
  bool ret = pressed_count > 0;
  pressed_count = 0;
  return ret;
}

bool button ::is_held() {
  bool ret = held_count > 0;
  return ret;
}
