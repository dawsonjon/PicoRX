#ifndef __CLOCKS__H_
#define __CLOCKS__H_

#include <cstdint>

//We can get closer to the derired frequency if we allow small adjustments
//to the system clock. system clocks in the range 125 - 133 MHz are fast
//enough to run the software. There are nearly 50 different frequencies in this
//range, by chosing the frequency that gives the best match, we can get within about 4Khz.
struct PLLSettings{
  uint32_t frequency;
  uint8_t refdiv;
  uint16_t fbdiv;
  uint8_t postdiv1;
  uint8_t postdiv2;
};

extern PLLSettings possible_frequencies[];
extern uint16_t num_possible_frequencies;

#endif
