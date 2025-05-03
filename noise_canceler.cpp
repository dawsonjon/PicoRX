#include "noise_canceler.h"

#include <string.h>

#include "rx_definitions.h"

#define MAG_SIZE (new_fft_size)
#define NUM_FRAMES (10)

#define LAMBDA (1638)    // 0.05
#define FIX_ONE (32767)  // 1.0

const uint16_t update_freq = adc_sample_rate / adc_block_size;
const uint16_t frame_switch_tres = (update_freq);
const float tframe = (float)adc_block_size / adc_sample_rate;
const float tau = 30e-3;  // 30ms
const uint16_t k = FIX_ONE * expf(-tframe / tau);
const uint16_t one_minus_k = FIX_ONE - k;

static uint16_t frames[NUM_FRAMES][MAG_SIZE];
static uint16_t lpf_mags[MAG_SIZE];
static uint16_t frame_idx;
static uint16_t count;
static nc_mode_e mode = nc_mode_soft;

static inline uint16_t mag_min(uint16_t m1, uint16_t m2) {
  return m1 < m2 ? m1 : m2;
}

void noise_canceler_init(void) {
  for (uint16_t i = 0; i < NUM_FRAMES; i++) {
    for (uint16_t j = 0; j < MAG_SIZE; j++) {
      frames[i][j] = FIX_ONE;
    }
  }
}

void noise_canceler_set_mode(nc_mode_e m) { mode = m; }

void noise_canceler_update(uint16_t mags[]) {
  for (uint16_t i = 0; i < MAG_SIZE; i++) {
    if (mode == nc_mode_soft) {
      lpf_mags[i] = ((one_minus_k * (uint32_t)mags[i]) >> 16) +
                    ((k * (uint32_t)lpf_mags[i]) >> 16);
      mags[i] = lpf_mags[i];
    }

    frames[frame_idx][i] = mag_min(frames[frame_idx][i], mags[i]);

    // Minimum Magnitude Spectral Estimate
    uint32_t mmse = frames[0][i];

    for (uint16_t j = 1; j < NUM_FRAMES; j++) {
      mmse = mag_min(mmse, frames[j][i]);
    }

    if (mags[i] > 0) {
      const uint16_t alpha = (mode == nc_mode_soft) ? 8 : 100;
      const uint32_t r = ((alpha * mmse) << 16) / mags[i];
      const uint32_t s =
          (mode == nc_mode_soft) ? ((LAMBDA * mmse) / mags[i]) : LAMBDA;
      if (r > (FIX_ONE - s)) {
        mags[i] = s;
      } else {
        mags[i] = FIX_ONE - r;
      }
    }
  }

  count++;
  if (count == frame_switch_tres) {
    frame_idx = (frame_idx + 1) % NUM_FRAMES;
    count = 0;
    for (uint16_t i = 0; i < MAG_SIZE; i++) {
      frames[frame_idx][i] = FIX_ONE;
    }
  }
}
