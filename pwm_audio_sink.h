#pragma once

#include <stdint.h>
#include <rx_definitions.h>
 
#define PWM_AUDIO_NUM_SAMPLES (adc_block_size / total_decimation_rate)

void pwm_audio_sink_init(void);
void pwm_audio_sink_start(void);
void pwm_audio_sink_stop(void);
uint32_t pwm_audio_sink_push(int16_t samples[PWM_AUDIO_NUM_SAMPLES], int16_t gain);
void pwm_audio_sink_update_pwm_max(uint32_t new_max);
