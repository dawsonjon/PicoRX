#include "pwm_audio_sink.h"
#include "pins.h"

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/sync.h"


#define NUM_OUT_SAMPLES (PWM_AUDIO_NUM_SAMPLES * interpolation_rate)

static int audio_pwm_slice_num;
static int pwm_dma_ping;
static int pwm_dma_pong;
static dma_channel_config audio_ping_cfg;
static dma_channel_config audio_pong_cfg;

static int16_t ping_audio[NUM_OUT_SAMPLES];
static int16_t pong_audio[NUM_OUT_SAMPLES];

static uint32_t pwm_max;
static uint32_t pwm_scale;

static void interpolate(int16_t sample, int16_t pwm_samples[], int16_t gain) {

  // digital volume control
  sample = ((int32_t)sample * gain) >> 8;
   
  // shift up
  sample += INT16_MAX;
  sample = (uint16_t)sample / pwm_scale;

  // interpolate to PWM rate
  static int16_t last_sample = 0;
  int32_t comb = sample - last_sample;
  last_sample = sample;
  for (uint8_t subsample = 0; subsample < interpolation_rate; ++subsample) {
    static int32_t integrator = 0;
    integrator += comb;
    pwm_samples[subsample] = integrator / interpolation_rate;
  }
}

void pwm_audio_sink_init(void) {
  gpio_set_function(PIN_AUDIO, GPIO_FUNC_PWM);
  gpio_set_drive_strength(PIN_AUDIO, GPIO_DRIVE_STRENGTH_12MA);
  audio_pwm_slice_num = pwm_gpio_to_slice_num(PIN_AUDIO);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 1.f);
  pwm_config_set_wrap(&config, pwm_max);
  pwm_init(audio_pwm_slice_num, &config, true);

  pwm_dma_ping = dma_claim_unused_channel(true);
  pwm_dma_pong = dma_claim_unused_channel(true);
  audio_ping_cfg = dma_channel_get_default_config(pwm_dma_ping);
  audio_pong_cfg = dma_channel_get_default_config(pwm_dma_pong);

  channel_config_set_transfer_data_size(&audio_ping_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&audio_ping_cfg, true);
  channel_config_set_write_increment(&audio_ping_cfg, false);
  channel_config_set_dreq(&audio_ping_cfg,
                          DREQ_PWM_WRAP0 + audio_pwm_slice_num);

  channel_config_set_transfer_data_size(&audio_pong_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&audio_pong_cfg, true);
  channel_config_set_write_increment(&audio_pong_cfg, false);
  channel_config_set_dreq(&audio_pong_cfg,
                          DREQ_PWM_WRAP0 + audio_pwm_slice_num);
}

void pwm_audio_sink_start(void) {
  dma_channel_configure(pwm_dma_ping, &audio_ping_cfg,
                        &pwm_hw->slice[audio_pwm_slice_num].cc, ping_audio,
                        NUM_OUT_SAMPLES, false);
  dma_channel_configure(pwm_dma_pong, &audio_pong_cfg,
                        &pwm_hw->slice[audio_pwm_slice_num].cc, pong_audio,
                        NUM_OUT_SAMPLES, false);
}

void pwm_audio_sink_stop(void) {
  dma_channel_cleanup(pwm_dma_ping);
  dma_channel_cleanup(pwm_dma_pong);
}

uint32_t pwm_audio_sink_push(int16_t samples[PWM_AUDIO_NUM_SAMPLES], int16_t gain) {
  static bool toggle = false;
  uint32_t time;

  if (toggle) {
    for (uint16_t i = 0; i < PWM_AUDIO_NUM_SAMPLES; i++) {
      interpolate(samples[i], &ping_audio[i * interpolation_rate], gain);
    }
    time = time_us_32();
    dma_channel_wait_for_finish_blocking(pwm_dma_pong);
    dma_channel_set_read_addr(pwm_dma_ping, ping_audio, true);
  } else {
    for (uint16_t i = 0; i < PWM_AUDIO_NUM_SAMPLES; i++) {
      interpolate(samples[i], &pong_audio[i * interpolation_rate], gain);
    }
    time = time_us_32();
    dma_channel_wait_for_finish_blocking(pwm_dma_ping);
    dma_channel_set_read_addr(pwm_dma_pong, pong_audio, true);
  }
  toggle ^= 1;
  return time;
}

void disable_pwm()
{
  gpio_disable_pulls(PIN_AUDIO);
  gpio_set_dir(PIN_AUDIO, false);
}

void enable_pwm()
{
  gpio_set_function(PIN_AUDIO, GPIO_FUNC_PWM);
  gpio_set_drive_strength(PIN_AUDIO, GPIO_DRIVE_STRENGTH_12MA);
}

void pwm_audio_sink_update_pwm_max(uint32_t new_max) {
  pwm_max = new_max;
  pwm_set_wrap(audio_pwm_slice_num, pwm_max);
  pwm_scale = 1 + ((INT16_MAX * 2) / pwm_max);
}
