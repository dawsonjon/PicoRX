#include "noise_canceler.h"

#include <string.h>

#include "rx_definitions.h"

#define MAG_SIZE (new_fft_size)
#define NUM_FRAMES (10)

#define ALPHA (100)      // 100.0
#define LAMBDA (1638)    // 0.05
#define FIX_ONE (32767)  // 1.0

const uint16_t update_freq = adc_sample_rate / adc_block_size;
const uint16_t frame_switch_tres = (update_freq);

static uint16_t frames[NUM_FRAMES][MAG_SIZE];
static uint16_t frame_idx;
static uint16_t count;

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

void noise_canceler_update(uint16_t mags[]) {
  for (uint16_t i = 0; i < MAG_SIZE; i++) {
    frames[frame_idx][i] = mag_min(frames[frame_idx][i], mags[i]);

    // Minimum Magnitude Spectral Estimate
    uint32_t mmse = frames[0][i];

    for (uint16_t j = 1; j < NUM_FRAMES; j++) {
      mmse = mag_min(mmse, frames[j][i]);
    }

    if (mags[i] > 0) {
      const uint32_t r = ((ALPHA * mmse) << 16) / mags[i];
      const uint32_t s = (LAMBDA * mmse) / mags[i];
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
