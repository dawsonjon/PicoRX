#ifndef NCO_H_
#define NCO_H_
#include "hardware/pio.h"
float nco_set_frequency(PIO pio, uint sm, float tuned_frequency, uint32_t &system_clock_frequency_out);

#endif
