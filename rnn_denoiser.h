#pragma once

#include <stdint.h>

#define RNND_NFFT (64)
typedef float rnn_num_t;

void rnn_denoiser_denoise(uint16_t x[RNND_NFFT], rnn_num_t g[RNND_NFFT], uint8_t th);
