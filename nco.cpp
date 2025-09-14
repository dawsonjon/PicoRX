#include "nco.h"

#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <cmath>
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
