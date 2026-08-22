#pragma once

#include <stdint.h>
#include <stddef.h>

void fft_iq_correct(int16_t *i, int16_t *q, uint16_t start_bin, uint16_t stop_bin);
