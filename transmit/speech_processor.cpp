#include "speech_processor.h"
#include <cstdio>
#include <cmath>
#ifndef SIMULATION
#include "pico/stdlib.h"
#endif


#ifndef SIMULATION
int16_t __not_in_flash_func(noise_gate)(int16_t x) {
#else
int16_t noise_gate(int16_t x) {
#endif
    static float magnitude = 0.0f;
    static float smoothed_gain = 0.0f;
    static bool gate_open = false;
    magnitude = 0.9f * magnitude + 0.1f * fabs(x);

    if (gate_open)
    {
      if (magnitude < 800) gate_open = false;
      smoothed_gain = 0.95f * smoothed_gain + 0.05f;
    }
    else
    {
      if (magnitude > 2000) gate_open = true;
      smoothed_gain = 0.99f * smoothed_gain;
    }

    return x * smoothed_gain;
}

#ifndef SIMULATION
int16_t __not_in_flash_func(treble)(int16_t x, int8_t level) {
#else
int16_t treble(int16_t x, int8_t level) {
#endif
  uint8_t idx = level + 5;
  static const int32_t treble_taps[11][2][3] = {
   { {6958, -4956, 1828}, {16383, -19812, 7260} },
   { {8253, -6351, 2297}, {16383, -19127, 6942} },
   { {9793, -8081, 2885}, {16383, -18412, 6626} },
   { {11623, -10221, 3623}, {16383, -17669, 6312} },
   { {13799, -12857, 4546}, {16383, -16896, 6001} },
   { {16383, -16095, 5696}, {16383, -16095, 5696} },
   { {19451, -20061, 7125}, {16383, -15265, 5397} },
   { {23092, -24904, 8896}, {16383, -14406, 5107} },
   { {27408, -30803, 11085}, {16383, -13520, 4827} },
   { {32521, -37967, 13781}, {16383, -12607, 4559} },
   { {38573, -46647, 17093}, {16383, -11668, 4304} },
  };

  static int16_t x1 = 0;
  static int16_t y1 = 0;
  static int16_t x2 = 0;
  static int16_t y2 = 0;
  static int16_t err = 0;

  x >>= 1;
  int32_t y = ((int32_t)x * treble_taps[idx][0][0]) +
              ((int32_t)x1 * treble_taps[idx][0][1]) +
              ((int32_t)x2 * treble_taps[idx][0][2]) + err;
  y -= ((int32_t)y1 * treble_taps[idx][1][1]) +
       ((int32_t)y2 * treble_taps[idx][1][2]);

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
int16_t __not_in_flash_func(bass)(int16_t x, int8_t level) {
#else
int16_t bass(int16_t x, int8_t level) {
#endif
  uint8_t idx = level + 5;
  static const int32_t bass_taps[11][2][3] = {
   { {12775, -17118, 6504}, {16383, -14782, 5232} },
   { {13435, -17518, 6584}, {16383, -15628, 5526} },
   { {14123, -17884, 6650}, {16383, -16446, 5827} },
   { {14842, -18210, 6702}, {16383, -17235, 6135} },
   { {15594, -18492, 6739}, {16383, -17995, 6448} },
   { {16383, -18725, 6763}, {16383, -18725, 6763} },
   { {17212, -18905, 6774}, {16383, -19427, 7080} },
   { {18084, -19025, 6772}, {16383, -20101, 7398} },
   { {19005, -19078, 6760}, {16383, -20746, 7714} },
   { {19979, -19058, 6739}, {16383, -21363, 8029} },
   { {21010, -18957, 6710}, {16383, -21953, 8341} },

   };

  static int16_t x1 = 0;
  static int16_t y1 = 0;
  static int16_t x2 = 0;
  static int16_t y2 = 0;
  static int16_t err = 0;

  x >>= 1;
  int32_t y = ((int32_t)x * bass_taps[idx][0][0]) +
              ((int32_t)x1 * bass_taps[idx][0][1]) +
              ((int32_t)x2 * bass_taps[idx][0][2]) + err;
  y -= ((int32_t)y1 * bass_taps[idx][1][1]) +
       ((int32_t)y2 * bass_taps[idx][1][2]);

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
int16_t __not_in_flash_func(process_speech)(int16_t x, bool enable_noise_gate, int8_t treble_level, int8_t bass_level) {
#else
int16_t process_speech(int16_t x, bool enable_noise_gate, int8_t treble_level, int8_t bass_level) {
#endif
  if(enable_noise_gate) x = noise_gate(x);
  x = bass(x, bass_level);
  x = treble(x, treble_level);
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
