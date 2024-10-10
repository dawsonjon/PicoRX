#ifndef RX_CONSTANTS
#define RX_CONSTANTS
#include <math.h>

const uint32_t adc_sample_rate = 480e3;
const uint32_t audio_sample_rate = adc_sample_rate/2;
const uint8_t  adc_bits = 12u;
const uint16_t adc_max=1<<(adc_bits-1);
const uint16_t adc_block_size = 2048u;
const uint8_t  AM = 0u;
const uint8_t  AMSYNC = 1u;
const uint8_t  LSB = 2u;
const uint8_t  USB = 3u;
const uint8_t  FM = 4u;
const uint8_t  CW = 5u;

const uint16_t decimation_rate = 32u; //cic decimation
const uint16_t cic_decimation_rate = decimation_rate/2u;
const uint16_t interpolation_rate = decimation_rate/2u;
const uint16_t extra_bits = 1u;
const uint8_t  cic_order = 4u;
const uint8_t  cic_bit_growth = ceilf(cic_order*log2f(cic_decimation_rate));
const float    cic_gain = powf(2.0f, (cic_order*log2f(cic_decimation_rate)) - (cic_bit_growth - extra_bits));

const float full_scale_signal_strength = 0.707f*adc_max*(1<<extra_bits);
const float full_scale_rms_mW = (0.5f * 0.707f * 1000.0f * 3.3f * 3.3f) / 50.0f;
const float full_scale_dBm = 10.0f * log10f(full_scale_rms_mW);
const float S0 = -127.0f;
const float S1 = -121.0f;
const float S9 = -73.0f;
const float S9_10 = -63.0f;

#endif
