#ifndef RX_DSP_H
#define RX_DSP_H

#include <stdint.h>
#include "half_band_filter.h"
#include "half_band_filter2.h"
#include <math.h>

class rx_dsp
{
  public:
  rx_dsp(double offset_frequency);
  uint16_t process_block(uint16_t samples[], int16_t audio_samples[]);
  static const uint16_t block_size = 2048;
  static const uint16_t decimation_rate = 20;
  static const uint16_t growth = ceil(log2(decimation_rate)) * 4;

  private:

  //used in dc canceler
  int32_t dc;

  //used in frequency shifter
  uint32_t phase;
  uint32_t frequency;
  int16_t sin_table[1024];
  int16_t cos_table[1024];

  //used in CIC filter
  uint8_t decimate;
  int32_t integratori1, integratorq1;
  int32_t integratori2, integratorq2;
  int32_t integratori3, integratorq3;
  int32_t integratori4, integratorq4;
  int32_t delayi0, delayq0;
  int32_t delayi1, delayq1;
  int32_t delayi2, delayq2;
  int32_t delayi3, delayq3;

  //used in half band filter
  half_band_filter half_band_filter_inst;
  half_band_filter2 half_band_filter2_inst;

  //removes dc component of AM signal
  int32_t audio_dc=0;

  //test tone
  int16_t signal = 0;

  //used in AGC
  uint8_t attack_factor;
  uint8_t decay_factor;
  uint16_t hang_time;
  uint16_t hang_timer;
  const uint8_t agc_setting = 3;
  const bool agc_enabled = true;
  int32_t max_hold;

};

#endif
