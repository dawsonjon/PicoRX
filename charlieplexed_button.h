#ifndef __button_charlieplexed__
#define __button_charlieplexed__

#include <cstdint>

class charlieplexed_button
{
  public:
  charlieplexed_button(uint8_t gpio_num, uint8_t gpio_enable_num);
  bool is_keyed();
  bool is_pressed();
  bool is_held();
  private:
  uint8_t gpio_num;
  uint8_t gpio_enable_num;
  void update_state();

  enum e_button_state {up, down, pressed, held};
  e_button_state button_state = up;
  uint32_t time_pressed = 0;

};

#endif
