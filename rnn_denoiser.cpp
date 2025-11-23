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

#define MEL_BINS (8)
#define FB_LEN (10)

const uint8_t FILTERBANK_OFF[MEL_BINS] = {1, 2, 4, 6, 8, 10, 13, 16};

const float FILTERBANK_MAT[MEL_BINS][FB_LEN] = {
    {2.60904729e-01, 5.00000000e-01, 2.39095271e-01, 0.00000000e+00,
     0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
     0.00000000e+00, 0.00000000e+00},
    {1.09047275e-02, 2.71809459e-01, 4.89095271e-01, 2.28190541e-01,
     0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
     0.00000000e+00, 0.00000000e+00},
    {2.18094569e-02, 2.82714188e-01, 4.78190541e-01, 2.17285827e-01,
     0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
     0.00000000e+00, 0.00000000e+00},
    {3.19565535e-02, 2.86818981e-01, 4.59365487e-01, 2.21858993e-01,
     0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
     0.00000000e+00, 0.00000000e+00},
    {3.27486321e-02, 2.28633225e-01, 4.01388615e-01, 2.46206030e-01,
     9.10234749e-02, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00,
     0.00000000e+00, 0.00000000e+00},
    {8.43757857e-03, 1.36507601e-01, 2.64577627e-01, 2.97906786e-01,
     1.96825728e-01, 9.57446843e-02, 0.00000000e+00, 0.00000000e+00,
     0.00000000e+00, 0.00000000e+00},
    {3.19443345e-02, 1.09208584e-01, 1.86472848e-01, 2.56438643e-01,
     1.95456743e-01, 1.34474859e-01, 7.34929517e-02, 1.25110541e-02,
     0.00000000e+00, 0.00000000e+00},
    {2.57221330e-03, 5.12947105e-02, 1.00017212e-01, 1.48739710e-01,
     1.97462216e-01, 1.76892623e-01, 1.38437703e-01, 9.99827906e-02,
     6.15278669e-02, 2.30729505e-02},
};

const float MEAN_VEC[MEL_BINS] = {
    -2.02434206e+00, -2.01172900e+00, -2.09666348e+00, -2.18149567e+00,
    -2.21266031e+00, -2.21795726e+00, -2.24682999e+00, -2.22735381e+00};

const float STD_VEC[MEL_BINS] = {4.52805787e-01, 4.75480765e-01, 4.61446911e-01,
                                 4.47916299e-01, 4.12093073e-01, 3.97672981e-01,
                                 3.64583939e-01, 3.56304556e-01};

const uint8_t MEL_IDX[MEL_BINS] = {0, 3, 5, 8, 10, 14, 19, 26};

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
    // output[j] = std::clamp(output[j], -1.0f, 1.0f);
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

static uint32_t __time_critical_func(avg)(uint16_t x[RNND_NFFT]) {
  static uint32_t avg = 0;
  uint32_t m = std::accumulate(x, x + RNND_NFFT, 0);
  m /= RNND_NFFT;
  avg += m - (avg / 256);

  return avg;
}

void rnn_denoiser_denoise(uint16_t x[RNND_NFFT], rnn_num_t g[RNND_NFFT], uint8_t th) {
  rnn_num_t out[GRNN0_HIDD_DIM1] = {0};
  rnn_num_t input[MEL_BINS];

  const uint32_t a = avg(x);
  for (uint16_t i = 0; i < MEL_BINS; i++) {
    rnn_num_t s = 0;
    for (uint16_t j = 0; j < FB_LEN; j++) {
      if (FILTERBANK_MAT[i][j] > 0) {
        s += FILTERBANK_MAT[i][j] * x[FILTERBANK_OFF[i] + j];
      }
    }
    input[i] = ((log10f(1e-8f + (s / (a / 8))) - MEAN_VEC[i]) / STD_VEC[i]);
  }

  rnn_process(input, out);
  fc_process(out, input);

  memset(g, 0, RNND_NFFT * sizeof(rnn_num_t));
  const rnn_num_t g_sum = std::accumulate(input, input + MEL_BINS, 0.0f);
  if (g_sum < ((2 * th) / 10.0f)) {
    return;
  }

  // interpolate gains
  int16_t x1 = MEL_IDX[0];
  rnn_num_t y1 = input[0];
  uint16_t k = 0;

  for (uint16_t i = 1; i < MEL_BINS + 1; i++) {
    const int16_t x0 = x1;
    const rnn_num_t y0 = y1;
    if (i < MEL_BINS) {
      y1 = input[i];
      x1 = MEL_IDX[i];
    } else {
      x1++;
    }
    const rnn_num_t d = (y1 - y0) / (x1 - x0);
    for (uint16_t j = 0; j < (x1 - x0); j++) {
      g[k] = std::clamp(y0 + j * d, 0.0f, 1.0f);
      k++;
    }
  }
}
