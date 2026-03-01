//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: iq_pwm.cpp
// description: PWM iq for Ham Transmitter
// License: MIT
//
//


#include "iq_pwm.h"
#include "stdio.h"

const uint8_t interpolation_ratio = 16;

iq_pwm::iq_pwm(const uint8_t i_pin,
               const uint8_t q_pin,
               const uint32_t audio_sample_rate,
               const uint32_t cpuFrequencyHz)
{

  m_i_pin = i_pin;
  m_q_pin = q_pin;

  // ----- configure LEFT PWM pin -----
  gpio_set_function(i_pin, GPIO_FUNC_PWM);
  gpio_set_drive_strength(i_pin, GPIO_DRIVE_STRENGTH_12MA);
  i_slice = pwm_gpio_to_slice_num(i_pin);

  // ----- configure RIGHT PWM pin -----
  gpio_set_function(q_pin, GPIO_FUNC_PWM);
  gpio_set_drive_strength(q_pin, GPIO_DRIVE_STRENGTH_12MA);
  q_slice = pwm_gpio_to_slice_num(q_pin);

  // ----- configure PWM (same for both slices) -----
  const uint16_t pwm_max = 255;   // 8-bit PWM
  pwm_config config_i = pwm_get_default_config();
  pwm_config config_q = pwm_get_default_config();
  pwm_config_set_clkdiv(&config_i, 1.f);
  pwm_config_set_clkdiv(&config_q, 1.f);
  pwm_config_set_wrap(&config_i, pwm_max);
  pwm_config_set_wrap(&config_q, pwm_max);

  pwm_init(i_slice, &config_i, true);
  pwm_init(q_slice, &config_q, true);

  // ----- claim DMA channels -----
  i_dma = dma_claim_unused_channel(true);
  q_dma = dma_claim_unused_channel(true);

  dma_channel_config cfg_i = dma_channel_get_default_config(i_dma);
  dma_channel_config cfg_q = dma_channel_get_default_config(q_dma);

  // same config for both channels
  channel_config_set_transfer_data_size(&cfg_i, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg_i, true);
  channel_config_set_write_increment(&cfg_i, false);

  channel_config_set_transfer_data_size(&cfg_q, DMA_SIZE_16);
  channel_config_set_read_increment(&cfg_q, true);
  channel_config_set_write_increment(&cfg_q, false);

  // ----- claim ONE timer shared by both DMAs -----
  dma_timer = dma_claim_unused_timer(true);
  dma_timer_set_fraction(dma_timer, 1, cpuFrequencyHz / (interpolation_ratio*audio_sample_rate));

  // both DMAs use same pacing source
  uint dreq = dma_get_timer_dreq(dma_timer);
  channel_config_set_dreq(&cfg_i, dreq);
  channel_config_set_dreq(&cfg_q, dreq);

  // ----- configure write addresses (PWM CC registers) -----
  dma_channel_configure(
      i_dma,
      &cfg_i,
      &pwm_hw->slice[i_slice].cc,   // write address
      NULL,                         // read addr set later
      interpolation_ratio,
      false);

  dma_channel_configure(
      q_dma,
      &cfg_q,
      &pwm_hw->slice[q_slice].cc,
      NULL,
      interpolation_ratio,
      false);
}

iq_pwm::~iq_pwm() {

  // ----- stop DMA channels if active -----
  if (dma_channel_is_claimed(i_dma))
  {
    dma_channel_abort(i_dma);
    dma_channel_unclaim(i_dma);
  }

  if (dma_channel_is_claimed(q_dma))
  {
    dma_channel_abort(q_dma);
    dma_channel_unclaim(q_dma);
  }

  // ----- release pacing timer -----
  dma_timer_unclaim(dma_timer);

  // ----- disable PWM slices -----
  pwm_set_enabled(i_slice, false);
  pwm_set_enabled(q_slice, false);

  // ----- return GPIOs to inputs (safe state) -----
  gpio_set_function(m_i_pin, GPIO_FUNC_NULL);
  gpio_set_dir(m_i_pin, GPIO_IN);

  gpio_set_function(m_q_pin, GPIO_FUNC_NULL);
  gpio_set_dir(m_q_pin, GPIO_IN);
}

void __not_in_flash_func(iq_pwm::output_sample)(int16_t i, int16_t q) {
  dma_channel_wait_for_finish_blocking(i_dma);
  dma_channel_wait_for_finish_blocking(q_dma);

  static uint16_t i_samples[interpolation_ratio];
  static uint16_t q_samples[interpolation_ratio];

  static int32_t last_i = 0;
  static int32_t last_q = 0;
  int32_t comb_i = i - last_i;
  int32_t comb_q = q - last_q;
  last_i = i;
  last_q = q;
  for (uint8_t subsample = 0; subsample < interpolation_ratio; ++subsample) {
    static int32_t integrator_i = 0;
    static int32_t integrator_q = 0;
    integrator_i += comb_i;
    integrator_q += comb_q;

    static int32_t residue_i = 0;
    static int32_t residue_q = 0;
    const int32_t i_interpolated = residue_i + (integrator_i / interpolation_ratio);
    const int32_t q_interpolated = residue_q + (integrator_q / interpolation_ratio);


    const int16_t i_quantized = i_interpolated >> 8;
    const int16_t q_quantized = q_interpolated >> 8;
    residue_i = i_interpolated - (i_quantized << 8);
    residue_q = q_interpolated - (q_quantized << 8);

    i_samples[subsample] = i_quantized + 128;
    q_samples[subsample] = q_quantized + 128;
  }

  dma_channel_set_read_addr(i_dma, i_samples, true);
  dma_channel_set_read_addr(q_dma, q_samples, true);

}
