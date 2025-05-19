#pragma once

#include <stdint.h>
#include <rx_definitions.h>
 
#define PWM_AUDIO_NUM_SAMPLES (adc_block_size / decimation_rate)

typedef enum { linear = 0, polyphase } pwm_audio_interp_mode_e;

void pwm_audio_sink_init(void);
void pwm_audio_sink_start(void);
void pwm_audio_sink_set_interpolation_mode(pwm_audio_interp_mode_e mode);
void pwm_audio_sink_stop(void);
void pwm_audio_sink_push(int16_t samples[PWM_AUDIO_NUM_SAMPLES], int16_t gain);
void pwm_audio_sink_update_pwm_max(uint32_t new_max);
