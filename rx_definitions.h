#ifndef RX_CONSTANTS
#define RX_CONSTANTS
#include <math.h>

static const uint16_t decimation_rate = 20; //cic only decimation
static const uint16_t total_decimation_rate = decimation_rate * 2; //include half band decimator
static const uint16_t interpolation_rate = 20;
static const uint16_t adc_block_size = 4000;
static const uint16_t pwm_block_size = (adc_block_size * interpolation_rate) / total_decimation_rate;
static const uint16_t growth = ceil(log2(decimation_rate)) * 4;

#endif
