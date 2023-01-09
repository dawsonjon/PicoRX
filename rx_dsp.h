#include <stdint.h>
#include "half_band_filter.h"
#include "half_band_filter2.h"
#include <math.h>

class rx_dsp
{
  public:
  rx_dsp(double offset_frequency);
  uint16_t process_block(uint16_t samples[], int16_t i_samples[], int16_t q_samples[]);
  static const uint16_t block_size = 1000;
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

};
