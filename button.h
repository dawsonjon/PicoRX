#ifndef __button__
#define __button__

#include <cstdint>

class button
{
  public:
  button(uint8_t gpio_num);
  bool is_pressed();
  bool is_held();
  void update_state();
  bool is_keyed();

  private:
  uint8_t gpio_num;

  enum e_button_state {idle, decoding, active};
  e_button_state button_state = idle;
  uint32_t time_pressed = 0;
  uint8_t pressed_count;
  uint8_t held_count;

};

#endif
