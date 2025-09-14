#include "nco.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include <cmath>
extern "C" {
  #include "si5351.h"
}

float nco_set_frequency(PIO pio, uint sm, float tuned_frequency, uint32_t &system_clock_frequency_out) {
    // Calculate the best frequency to set the Si5351 to
  si5351_set_freq((uint64_t)(tuned_frequency*4.0*1000000.0), SI5351_CLK0);
  si5351_set_freq((uint64_t)(tuned_frequency*4.0*1000000.0), SI5351_CLK1);
  si5351_set_freq((uint64_t)(tuned_frequency*4.0*1000000.0), SI5351_CLK2);
  
  return tuned_frequency/4.0;
}
