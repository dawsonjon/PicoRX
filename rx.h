#ifndef RX__
#define RX__

#include <stdio.h>
#include <math.h>
#include <ctime>
#include "nco.pio.h"

#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

#include "rx_definitions.h"
#include "rx_dsp.h"

struct rx_settings
{
  double tuned_frequency_Hz;
  int step_Hz;
  uint8_t agc_speed;
  uint8_t mode;
  uint8_t volume;
  uint8_t squelch;
  uint8_t bandwidth;
  uint16_t cw_sidetone_Hz;
  uint16_t gain_cal;
  bool suspend;
  bool swap_iq;
};

struct rx_status
{
  int32_t signal_strength_dBm;
  clock_t busy_time;
  uint16_t temp;
  uint16_t battery;
};

class rx
{
  private:

  //receiver configuration
  double tuned_frequency_Hz;
  double nco_frequency_Hz;
  double offset_frequency_Hz;
  semaphore_t settings_semaphore;
  bool settings_changed;
  bool suspend;
  uint16_t temp;
  uint16_t battery;


  // Choose which PIO instance to use (there are two instances)
  PIO pio;
  uint offset;
  uint sm;

  //capture buffer DMA
  static int capture_dma;
  static dma_channel_config capture_cfg;

  //buffers and dma for adc
  static int adc_dma_ping;
  static int adc_dma_pong;
  static dma_channel_config ping_cfg;
  static dma_channel_config pong_cfg;
  static uint16_t ping_samples[adc_block_size];
  static uint16_t pong_samples[adc_block_size];
  static uint16_t num_ping_samples;
  static uint16_t num_pong_samples;

  //buffers and dma for PWM audio output
  static int audio_pwm_slice_num;
  static int pwm_dma_ping;
  static int pwm_dma_pong;
  static dma_channel_config audio_ping_cfg;
  static dma_channel_config audio_pong_cfg;
  static int16_t ping_audio[adc_block_size];
  static int16_t pong_audio[adc_block_size];
  static bool audio_running;
  static void dma_handler();
  uint32_t pwm_max;
  
  //store busy time for performance monitoring
  clock_t busy_time;

  public:
  rx(rx_settings & settings_to_apply, rx_status & status);
  void apply_settings();
  void run();
  void get_spectrum(float spectrum[]);
  rx_settings &settings_to_apply;
  rx_status &status;
  rx_dsp rx_dsp_inst;
  void read_batt_temp();
  void access(bool settings_changed);
  void release();
};

#endif
