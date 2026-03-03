#include "speech_processor.h"
#include "pico/stdlib.h"

int16_t __not_in_flash_func(treble_boost)(int16_t x) {
  static const int32_t treble_taps[2][3] = {
    {32521, -37967, 13781}, {16383, -12607, 4559}};

  static int16_t x1 = 0;
  static int16_t y1 = 0;
  static int16_t x2 = 0;
  static int16_t y2 = 0;
  static int16_t err = 0;

  x >>= 1;
  int32_t y = ((int32_t)x * treble_taps[0][0]) +
              ((int32_t)x1 * treble_taps[0][1]) +
              ((int32_t)x2 * treble_taps[0][2]) + err;
  y -= ((int32_t)y1 * treble_taps[1][1]) +
       ((int32_t)y2 * treble_taps[1][2]);

  if (y > 0xFFFFFFFL) {
    y = 0xFFFFFFFL;
  }
  if (y < -0x10000000L) {
    y = -0x10000000L;
  }

  err = y & ((1 << 14) - 1);
  y >>= 14;

  x2 = x1;
  x1 = x;
  y2 = y1;
  y1 = y;
  return y << 1;
}

int16_t __not_in_flash_func(high_pass)(int16_t x) {
  static int32_t low_pass = 0;
  low_pass = ((low_pass * 27570) + ((int32_t)x * 5198)) >> 15;
  int32_t high_pass = x - low_pass;
  if(high_pass > 32767) high_pass = 32767;
  if(high_pass < -32767) high_pass = -32767;
  return high_pass;
}

int16_t __not_in_flash_func(process_speech)(int16_t x) {
  x = high_pass(x);
  x = treble_boost(x);
  return x;
}
