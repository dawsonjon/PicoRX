#pragma once

#include "fft_filter.h"

void noise_canceler_init(void);
void noise_canceler_update(uint16_t mags[]);
