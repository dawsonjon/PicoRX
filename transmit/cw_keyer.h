#ifndef __CW_KEYER__
#define __CW_KEYER__

#include <cstdint>
#include "button.h"

const uint8_t STRAIGHT = 0;
const uint8_t IAMBIC_A = 1;
const uint8_t IAMBIC_B = 2;

enum e_keyer_state {IDLE, DIT, DAH, DIT_SPACE, DAH_SPACE, SPACE};

class cw_keyer
{

  uint16_t counter = 0;
  bool both_pressed = false;
  uint16_t dit_length_samples;
  uint8_t m_paddle_type;
  int8_t timer;
  e_keyer_state keyer_state = IDLE;

  bool dit_is_pressed();
  bool dah_is_pressed();
  bool get_straight();
  bool get_iambic();
  int16_t key_shape(bool on);

  button &dit;
  button &dah;

  public:
  cw_keyer(uint8_t paddle_type, uint8_t paris_wpm, uint32_t sample_rate_Hz, button &dit, button &dah);
  int16_t get_sample();

};

#endif
