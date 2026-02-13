#include "tx_best_clock.h"
#include "../clocks.h"

#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <cmath>
#include <cstdio>

double tx_best_clock(double wanted_frequency) {

    PLLSettings best_settings = {0, 0, 0, 0, 0};
    double best_error = 1000000.0;

    for(uint8_t idx = 0; idx < num_possible_frequencies; idx++)
    {

      uint32_t system_clock_frequency = possible_frequencies[idx].frequency;
      double ideal_divider;
      double nearest_divider;
      double actual_frequency;
      double error;

      //upper or nearest
      ideal_divider = system_clock_frequency/wanted_frequency;
      nearest_divider = round(ideal_divider);
      actual_frequency = system_clock_frequency/nearest_divider;
      error = abs(actual_frequency - wanted_frequency);
      if(error < best_error)
      {
        best_settings = possible_frequencies[idx];
        best_error = error;
      }

    }

    //adjust system clock
    uint32_t vco_freq = (12000000 / best_settings.refdiv) * best_settings.fbdiv;
    set_sys_clock_pll(vco_freq, best_settings.postdiv1, best_settings.postdiv2);

    //return actual frequency
    return best_settings.frequency;
}
