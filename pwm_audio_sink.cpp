#include "pwm_audio_sink.h"

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/sync.h"

#define AUDIO_PIN (16)

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
static pwm_audio_interp_mode_e interp_mode = pwm_audio_interp_mode_e::linear;

static const int16_t interp_h[4 * interpolation_rate] = {
    0,     39,    157,   353,   624,   967,   1381,  1859,  2399,  2995,  3641,
    4330,  5057,  5814,  6594,  7389,  8192,  8995,  9790,  10570, 11327, 12053,
    12743, 13389, 13984, 14524, 15003, 15416, 15760, 16031, 16226, 16344, 16384,
    16344, 16226, 16031, 15760, 15416, 15003, 14524, 13984, 13389, 12743, 12053,
    11327, 10570, 9790,  8995,  8192,  7389,  6594,  5814,  5057,  4330,  3641,
    2995,  2399,  1859,  1381,  967,   624,   353,   157,   39};

static void __time_critical_func(interpolate)(int16_t sample,
                                              int16_t pwm_samples[],
                                              int16_t gain) {
  // digital volume control
  sample = ((int32_t)sample * gain) >> 8;

  // shift up
  sample += INT16_MAX;
  sample = (uint16_t)sample / pwm_scale;

  // interpolate to PWM rate
  if (interp_mode == pwm_audio_interp_mode_e::linear) {
    static int16_t last_sample = 0;
    int32_t comb = sample - last_sample;
    last_sample = sample;
    for (uint8_t subsample = 0; subsample < interpolation_rate; ++subsample) {
      static int32_t integrator = 0;
      integrator += comb;
      pwm_samples[subsample] = integrator >> 4;
    }
  } else {
    // simple polyphase FIR interpolator
    static int16_t sample_n_1 = 0;
    static int16_t sample_n_2 = 0;
    static int16_t sample_n_3 = 0;
    for (uint8_t subsample = 0; subsample < interpolation_rate; ++subsample) {
      pwm_samples[subsample] =
          (interp_h[subsample] * (int32_t)sample +
           interp_h[subsample + interpolation_rate] * (int32_t)sample_n_1 +
           interp_h[subsample + (2 * interpolation_rate)] *
               (int32_t)sample_n_2 +
           interp_h[subsample + (3 * interpolation_rate)] *
               (int32_t)sample_n_3) >>
          15;
    }
    sample_n_3 = sample_n_2;
    sample_n_2 = sample_n_1;
    sample_n_1 = sample;
  }
}

void pwm_audio_sink_init(void) {
  gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
  gpio_set_drive_strength(AUDIO_PIN, GPIO_DRIVE_STRENGTH_12MA);
  audio_pwm_slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
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

void pwm_audio_sink_set_interpolation_mode(pwm_audio_interp_mode_e mode) {
  if (mode > pwm_audio_interp_mode_e::polyphase) {
    mode = pwm_audio_interp_mode_e::polyphase;
  }
  interp_mode = mode;
}

uint32_t pwm_audio_sink_push(int16_t samples[PWM_AUDIO_NUM_SAMPLES], int16_t gain) {
  static bool toggle = false;
  uint32_t tm = 0;

  if (toggle) {
    for (uint16_t i = 0; i < PWM_AUDIO_NUM_SAMPLES; i++) {
      interpolate(samples[i], &ping_audio[i << 4], gain);
    }
    tm = time_us_32();
    dma_channel_wait_for_finish_blocking(pwm_dma_pong);
    dma_channel_set_read_addr(pwm_dma_ping, ping_audio, true);
  } else {
    for (uint16_t i = 0; i < PWM_AUDIO_NUM_SAMPLES; i++) {
      interpolate(samples[i], &pong_audio[i << 4], gain);
    }
    tm = time_us_32();
    dma_channel_wait_for_finish_blocking(pwm_dma_ping);
    dma_channel_set_read_addr(pwm_dma_pong, pong_audio, true);
  }
  toggle ^= 1;
  return tm;
}

void pwm_audio_sink_update_pwm_max(uint32_t new_max) {
  pwm_max = new_max;
  pwm_scale = 1 + ((INT16_MAX * 2) / pwm_max);
  pwm_set_wrap(audio_pwm_slice_num, pwm_max);
  pwm_set_gpio_level(AUDIO_PIN, (pwm_max / 2));
}
