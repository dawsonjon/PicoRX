#ifndef RX_CONSTANTS
#define RX_CONSTANTS
#include <math.h>

const uint32_t adc_sample_rate = 500e3;
const uint32_t audio_sample_rate = adc_sample_rate/2;
const uint32_t system_clock_rate = 125e6;
const uint8_t  adc_bits = 12u;
const uint16_t adc_max=1<<(adc_bits-1);
const uint16_t cw_decimation_rate = 10u; //cic only decimation
const uint16_t cw_bit_growth = ceilf(log2f(cw_decimation_rate)*4.0f);
const uint32_t cw_bias_adjustment = (1.5f + 0.5f) * (1<<15); //0.5 for cic, 0.5 per half band filter, 0.5 for truncation in adjustment
const uint16_t adc_block_size = 4000u;
const uint32_t pwm_max = (system_clock_rate/audio_sample_rate)-1;
const uint32_t pwm_scale = 1+((INT16_MAX * 2)/pwm_max);
const uint16_t extra_bits=3; //When truncating after decimation, keep some extra LSBs since noise floor is now lower
const uint8_t  AM = 0u;
const uint8_t  LSB = 1u;
const uint8_t  USB = 2u;
const uint8_t  FM = 3u;
const uint8_t  CW = 4u;

const float full_scale_rms_mW = (0.5f * 0.707f * 1000.0f * 3.3f * 3.3f) / 50.0f;
const float full_scale_dBm = 10.0f * log10f(full_scale_rms_mW);
const float amplifier_gain_dB = 60.0f; //external amplifier gain
const float preamplifier_gain_dB = 14.0f; //external amplifier gain
const float filter_gain_dB = -12.0f; //external amplifier gain
const float S0 = -127.0f;
const float S1 = -121.0f;
const float S9 = -73.0f;
const float S9_10 = -63.0f;

#endif
