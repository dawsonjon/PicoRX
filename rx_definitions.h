#ifndef RX_CONSTANTS
#define RX_CONSTANTS
#include <math.h>

static const uint32_t adc_sample_rate = 500e3;
static const uint32_t system_clock_rate = 125e6;
static const uint8_t  adc_bits = 12u;
static const uint16_t decimation_rate = 20u; //cic only decimation
static const uint16_t cw_decimation_rate = 20u; //cic only decimation
static const uint16_t total_decimation_rate = decimation_rate * 2u; //include half band decimator
static const uint16_t interpolation_rate = 20u;
static const uint16_t adc_block_size = 4000u;
static const uint16_t pwm_block_size = (adc_block_size * interpolation_rate) / total_decimation_rate;
static const uint16_t growth = ceil(log2(decimation_rate)) * 4u;
static const uint32_t audio_sample_rate = (adc_sample_rate * interpolation_rate) / total_decimation_rate;
static const uint32_t pwm_max = (system_clock_rate/audio_sample_rate)-1;
static const uint32_t pwm_scale = 1+((INT16_MAX * 2)/pwm_max);
static const uint8_t  AM = 0u;
static const uint8_t  LSB = 1u;
static const uint8_t  USB = 2u;
static const uint8_t  FM = 3u;
static const uint8_t  CW = 4u;


#endif
