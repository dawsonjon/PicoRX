#include "noise_canceler.h"

#include <string.h>

#define MAG_SIZE (new_fft_size)
#define NUM_FRAMES (4)
#define FRAME_SWITCH_TRES \
  (586)  // 586 corresponds to 2.5s, multiplied by NUM_FRAMES gives about 10s
         // noise estimate

#define ALPHA (50)       // 50.0
#define LAMBDA (1638)    // 0.1
#define FIX_ONE (32767)  // 1.0

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
    if (mags[i] < frames[frame_idx][i]) {
      frames[frame_idx][i] = mags[i];
    }
    // Minimum Magnitude Spectral Estimate
    const uint32_t mmse = mag_min(mag_min(frames[0][i], frames[1][i]),
                                  mag_min(frames[2][i], frames[3][i]));
    if (mags[i] > 0) {
      const uint32_t r = ((ALPHA * mmse) << 16) / mags[i];
      if (r > (FIX_ONE - LAMBDA)) {
        mags[i] = LAMBDA;
      } else {
        mags[i] = FIX_ONE - r;
      }
    }
  }

  count++;
  if (count == 586) {
    frame_idx = (frame_idx + 1) % NUM_FRAMES;
    count = 0;
    for (uint16_t i = 0; i < MAG_SIZE; i++) {
      frames[frame_idx][i] = FIX_ONE;
    }
  }
}