#include "speech_processor.h"
#include <cstdio>
#ifndef SIMULATION
#include "pico/stdlib.h"
#endif

#ifndef SIMULATION
int16_t __not_in_flash_func(treble_boost)(int16_t x) {
#else
int16_t treble_boost(int16_t x) {
#endif
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

#ifndef SIMULATION
int16_t __not_in_flash_func(high_pass)(int16_t x) {
#else
int16_t high_pass(int16_t x) {
#endif
  static const int32_t treble_taps[2][3] = {
    {12572, -21999, 9779}, {16383, -20415, 7551}};
    //{10468, -20936, 10468}, {16383, -18725, 6763}};
    //{13116, -26232, 13116}, {16383, -25574, 10507}};

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

#ifndef SIMULATION
int16_t __not_in_flash_func(process_speech)(int16_t x) {
#else
int16_t process_speech(int16_t x) {
#endif
  x = high_pass(x);
  x = treble_boost(x);
  return x;
}

#ifndef SIMULATION
void __not_in_flash_func(compress)(int16_t &x, float &envelope, uint16_t limit) {
#else
void compress(int16_t &x, float &envelope, uint16_t limit) {
#endif

  static int16_t lookahead[16] = {0};
  static uint32_t inp = 0;
  int16_t x_del;

  lookahead[inp & 0xf] = x;
  x_del = lookahead[(inp+16) & 0xf] = x;
  inp++;

  uint16_t magnitude = x>0?x:-x;
  if(magnitude > envelope) envelope = magnitude;
  else envelope = (envelope * 0.995f) + (0.005f * magnitude);

  x = x_del;
  if(envelope > limit) {
      x = (float)x*limit/envelope;
      x = (int32_t)x*32767.0f/limit;
  }


}
