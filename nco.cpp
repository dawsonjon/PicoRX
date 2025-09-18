#include "nco.h"
#include "clocks.h"

#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <cmath>
#include <cstdio>

float nco_set_frequency(PIO pio, uint sm, float tuned_frequency, uint32_t &system_clock_frequency_out, uint8_t if_frequency_hz_over_100, uint8_t if_mode) {

    double adjusted_frequency_up = tuned_frequency + (if_frequency_hz_over_100*100);
    double adjusted_frequency_down = tuned_frequency - (if_frequency_hz_over_100*100);
    PLLSettings best_settings = {0};
    double best_frequency = 1.0;
    double best_divider = 0.0;
    double best_error = 1000000.0;

    for(uint8_t idx = 0; idx < num_possible_frequencies; idx++)
    {

      uint32_t system_clock_frequency = possible_frequencies[idx].frequency;
      double ideal_divider;
      double nearest_divider;
      double actual_frequency;
      double error;

      //upper or nearest
      if(if_mode == 1 || if_mode == 2)
      {
        ideal_divider = system_clock_frequency/(4.0*adjusted_frequency_up);
        nearest_divider = round(256.0*ideal_divider)/256.0;
        actual_frequency = system_clock_frequency/nearest_divider;
        error = abs(actual_frequency - 4.0*adjusted_frequency_up);
        if(error < best_error)
        {
          best_frequency = actual_frequency;
          best_settings = possible_frequencies[idx];
          best_divider = nearest_divider;
          best_error = error;
          system_clock_frequency_out = system_clock_frequency;
        }
      }

      //lower or nearest
      if(if_mode == 0 || if_mode == 2)
      {
        ideal_divider = system_clock_frequency/(4.0*adjusted_frequency_down);
        nearest_divider = round(256.0*ideal_divider)/256.0;
        actual_frequency = system_clock_frequency/nearest_divider;
        error = abs(actual_frequency - 4.0*adjusted_frequency_down);
        if(error < best_error)
        {
          best_frequency = actual_frequency;
          best_settings = possible_frequencies[idx];
          best_divider = nearest_divider;
          best_error = error;
          system_clock_frequency_out = system_clock_frequency;
        }
      }
    }

    assert(best_error < 1000000);
    //adjust system clock
    uint32_t vco_freq = (12000000 / best_settings.refdiv) * best_settings.fbdiv;
    set_sys_clock_pll(vco_freq, best_settings.postdiv1, best_settings.postdiv2);

    //set pio divider
    pio_sm_set_clkdiv(pio, sm, best_divider);

    //return actual frequency
    return best_frequency/4.0;
extern "C" {
  #include "si5351.h"
}

float nco_set_frequency(PIO pio, uint sm, float tuned_frequency, uint32_t &system_clock_frequency_out) {
  // Set Si5351 CLK0 and CLK1 to the desired frequency (in Hz), format: freq_Hz * 100ULL
  uint64_t freq = (uint64_t)(tuned_frequency) * 100ULL; // tuned_frequency is in Hz, library expects Hz*100
  printf("[DEBUG] nco_set_frequency: tuned_frequency = %f Hz, freq (for si5351) = %llu\n", tuned_frequency, freq);
  int r0 = si5351_set_freq(freq, SI5351_CLK0);
  printf("[DEBUG] si5351_set_freq CLK0: freq = %llu, ret = %d\n", freq, r0);
  int r1 = si5351_set_freq(freq, SI5351_CLK1);
  printf("[DEBUG] si5351_set_freq CLK1: freq = %llu, ret = %d\n", freq, r1);
  // Set CLK1 90 degree phase shift (quarter period)
  set_phase(SI5351_CLK0, 0); // 90° phase shift (256 = 360°, 64 = 90°)
  set_phase(SI5351_CLK1, 64); // set 90° phase shift (256/4 = 64)

  // Do not set CLK2
  return tuned_frequency;
}
