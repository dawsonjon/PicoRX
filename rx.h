#ifndef RX__
#define RX__

#include <stdio.h>
#include <math.h>
#include "nco.pio.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

#include "rx_definitions.h"
#include "rx_dsp.h"

class rx
{
  private:

  //receiver configuration
  double tuned_frequency_Hz;
  double nco_frequency_Hz;
  double offset_frequency_Hz;

  //receiver dsp
  rx_dsp rx_dsp_inst;

  // Choose which PIO instance to use (there are two instances)
  PIO pio;
  uint offset;
  uint sm;

  //buffers and dma for adc
  static int adc_dma_ping;
  static int adc_dma_pong;
  static dma_channel_config ping_cfg;
  static dma_channel_config pong_cfg;
  static uint16_t ping_samples[adc_block_size];
  static uint16_t pong_samples[adc_block_size];

  //buffers and dma for PWM audio output
  static int audio_pwm_slice_num;
  static int pwm_dma_ping;
  static int pwm_dma_pong;
  static dma_channel_config audio_ping_cfg;
  static dma_channel_config audio_pong_cfg;
  static int16_t ping_audio[pwm_block_size];
  static int16_t pong_audio[pwm_block_size];
  static bool audio_running;
  static void dma_handler();

  public:
  rx();
  void set_frequency_Hz(double);
  void run();
};

#endif
