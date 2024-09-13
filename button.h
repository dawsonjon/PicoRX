#ifndef __button__
#define __button__

#include <cstdint>

class button
{
  public:
  button(uint8_t gpio_num);
  bool is_pressed();
  bool is_held();
  private:
  uint8_t gpio_num;
  void update_state();

  enum e_button_state {up, down, pressed, held};
  e_button_state button_state = up;
  uint32_t time_pressed = 0;

};

#endif
