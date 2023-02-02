#ifndef RX_CONSTANTS
#define RX_CONSTANTS
#include <math.h>

static const uint32_t adc_sample_rate = 500e3;
static const uint32_t system_clock_rate = 125e6;
static const uint8_t  adc_bits = 12u;
static const uint16_t adc_max=1<<(adc_bits-1);
static const uint16_t decimation_rate = 20u; //cic only decimation
static const uint16_t cw_decimation_rate = 20u; //cic only decimation
static const uint16_t total_decimation_rate = decimation_rate * 2u; //include half band decimator
static const uint16_t interpolation_rate = 20u;
static const uint16_t adc_block_size = 4000u;
static const uint16_t pwm_block_size = (adc_block_size * interpolation_rate) / total_decimation_rate;
static const uint16_t bit_growth = ceilf(log2f(decimation_rate))*4; //extra bits needed to support bit growth
static const uint16_t extra_bits=3; //When truncating after decimation, keep some extra LSBs since noise floor is now lower
static const uint32_t audio_sample_rate = (adc_sample_rate * interpolation_rate) / total_decimation_rate;
static const uint32_t pwm_max = (system_clock_rate/audio_sample_rate)-1;
static const uint32_t pwm_scale = 1+((INT16_MAX * 2)/pwm_max);
static const uint8_t  AM = 0u;
static const uint8_t  LSB = 1u;
static const uint8_t  USB = 2u;
static const uint8_t  FM = 3u;
static const uint8_t  CW = 4u;

const float full_scale_rms_mW = (0.5f * 0.707f * 1000.0f * 3.3f * 3.3f) / 50.0f;
const float full_scale_dBm = 10.0f * log10f(full_scale_rms_mW);
const float amplifier_gain_dB = 60.0f; //external amplifier gain
const float growth_adjustment = decimation_rate/powf(2, ceil(log2f(decimation_rate))); //growth in decimating filters
const float full_scale_signal_strength = 0.707f*adc_max*(1<<extra_bits)*growth_adjustment; //signal strength after decimator expected for a full scale sin wave
const float S0 = -127.0f;
const float S1 = -121.0f;
const float S9 = -73.0f;
const float S9_10 = -63.0f;
const int16_t s9_threshold = full_scale_signal_strength*powf(10.0f, (S9 - full_scale_dBm + amplifier_gain_dB)/20.0f);

#endif
