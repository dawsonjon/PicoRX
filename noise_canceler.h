#pragma once

#include <stdint.h>

typedef enum {
  nc_mode_soft = 1,
  nc_mode_hard,
} nc_mode_e;

void noise_canceler_init(void);
void noise_canceler_set_mode(nc_mode_e m);
// 'mags' is reused as a return value for gain coefficients
void noise_canceler_update(uint16_t mags[]);
