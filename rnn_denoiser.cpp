#include "rnn_denoiser.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <iterator>
#include <numeric>

#include "fastgrnn_fc_params.h"
#include "fastgrnn_rnn_params.h"
#include "pico/stdlib.h"
#include "rnn_denoiser_cfg.h"

static inline rnn_num_t sigmoidf(rnn_num_t x) { return (1 / (1 + expf(-x))); }
static inline rnn_num_t hardsigmoid(rnn_num_t x) {
  if (x <= -3.0f) {
    return 0.0f;
  } else if (x >= 3.0f) {
    return 1.0f;
  } else {
    return (x / 6.0f) + 0.5f;
  }
}

static void __time_critical_func(_rnn_process)(
    const rnn_num_t input[GRNN0_HIDD_DIM0],
    const rnn_num_t hidden[GRNN0_HIDD_DIM1],
    rnn_num_t output[GRNN0_HIDD_DIM1]) {
  rnn_num_t z;
  rnn_num_t c;

  for (uint16_t j = 0; j < GRNN0_HIDD_DIM1; j++) {
    for (uint16_t i = 0; i < GRNN0_HIDD_DIM0; i++) {
      output[j] += (GRNN0_W[j][i] * input[i]);
    }
  }

  for (uint16_t j = 0; j < GRNN0_HIDD_DIM1; j++) {
    for (uint16_t i = 0; i < GRNN0_HIDD_DIM1; i++) {
      output[j] += (GRNN0_U[j][i] * hidden[i]);
    }
  }

  for (uint16_t j = 0; j < GRNN0_HIDD_DIM1; j++) {
    z = output[j] + GRNN0_BIAS_GATE[j];
    z = sigmoidf(z);
    c = output[j] + GRNN0_BIAS_UPDATE[j];
    c = tanhf(c);

    output[j] =
        z * hidden[j] + (GRNN0_SIGM_ZETA * (1.0 - z) + GRNN0_SIGM_NU) * c;
  }
}

static void __time_critical_func(fc_process)(const rnn_num_t input[FC_IN_DIM],
                                             rnn_num_t output[FC_OUT_DIM]) {
  memset(output, 0, FC_OUT_DIM * sizeof(rnn_num_t));

  for (size_t j = 0; j < FC_OUT_DIM; j++) {
    for (size_t i = 0; i < FC_IN_DIM; i++) {
      output[j] += (input[i] * FC_W[j][i]);
    }
    output[j] += FC_B[j];
    output[j] = hardsigmoid(output[j]);
  }
}

static void rnn_process(const rnn_num_t input[GRNN0_HIDD_DIM0],
                        rnn_num_t output[GRNN0_HIDD_DIM1]) {
  static rnn_num_t hidden[GRNN0_HIDD_DIM1] = {0};

  memset(output, 0, sizeof(rnn_num_t) * GRNN0_HIDD_DIM1);
  _rnn_process(input, hidden, output);
  memcpy(hidden, output, sizeof(rnn_num_t) * GRNN0_HIDD_DIM1);
}

static void __time_critical_func(interp)(rnn_num_t g_in[MEL_BINS],
                                         rnn_num_t g_out[RNND_NFFT]) {
  memset(g_out, 0, RNND_NFFT * sizeof(rnn_num_t));
  for (uint16_t i = 0; i < MEL_BINS; i++) {
    for (uint16_t j = 0; j < FB_LEN; j++) {
      if (FILTERBANK_MAT[i][j] > 0.0f) {
        g_out[FILTERBANK_OFF[i] + j] += 2 * g_in[i] * FILTERBANK_MAT[i][j];
      }
    }
  }
}

void rnn_denoiser_denoise(uint16_t x[RNND_NFFT], rnn_num_t g[RNND_NFFT]) {
  rnn_num_t out[GRNN0_HIDD_DIM1] = {0};
  rnn_num_t input[MEL_BINS];

  for (uint16_t i = 0; i < MEL_BINS; i++) {
    rnn_num_t s = 0;
    for (uint16_t j = 0; j < FB_LEN; j++) {
      if (FILTERBANK_MAT[i][j] > 0) {
        s += FILTERBANK_MAT[i][j] * (x[FILTERBANK_OFF[i] + j] << 1);
      }
    }
    rnn_num_t v = s / 32767;
    input[i] = ((log10f(1e-8f + v) - MEAN_VEC[i]) / STD_VEC[i]);
  }

  rnn_process(input, out);
  fc_process(out, input);
  interp(input, g);
}
