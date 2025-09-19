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
#include "hardware/sync.h"
#include "quadrature_si5351.h"

#include "button.h"
#include "rx_definitions.h"
#include "rx_dsp.h"

struct rx_settings
{
  double tuned_frequency_Hz;
  int step_Hz;
  uint8_t agc_setting;
  uint8_t agc_gain;
  uint8_t mode;
  uint8_t volume;
  uint8_t squelch_threshold;
  uint8_t squelch_timeout;
  uint8_t bandwidth;
  uint8_t deemphasis;
  uint8_t treble;
  uint8_t bass;
  uint16_t cw_sidetone_Hz;
  uint16_t gain_cal;
  uint8_t band_1_limit;
  uint8_t band_2_limit;
  uint8_t band_3_limit;
  uint8_t band_4_limit;
  uint8_t band_5_limit;
  uint8_t band_6_limit;
  uint8_t band_7_limit;
  int8_t ppm;
  bool suspend;
  bool swap_iq;
  bool iq_correction;
  bool enable_auto_notch;
  bool enable_noise_reduction;
  uint8_t noise_estimation;
  uint8_t noise_threshold;
  uint8_t if_frequency_hz_over_100;
  uint8_t if_mode;
  uint8_t spectrum_smoothing;
  bool enable_external_nco;
  bool stream_raw_iq;

  bool test_tone_enable;
  uint8_t test_tone_frequency;
  uint8_t cw_paddle;
  uint8_t cw_speed;
  uint8_t mic_gain;
  bool tx_modulation;
  uint8_t pwm_min;
  uint8_t pwm_max;
  uint8_t pwm_threshold;
};

struct rx_status
{
  int32_t signal_strength_dBm;
  uint32_t busy_time;
  uint16_t temp;
  uint16_t battery;
  s_filter_control filter_config;
  uint8_t usb_buf_level;
  uint16_t audio_level;
  float tuning_offset_Hz;
  bool transmitting;
};

class rx
{
  private:

  void update_status();
  void tx_update_status();
  void set_usb_callbacks();

  //receiver configuration
  uint32_t system_clock_rate;
  double tuned_frequency_Hz;
  double nco_frequency_Hz;
  double offset_frequency_Hz;
  semaphore_t settings_semaphore;
  bool settings_changed;
  volatile bool suspend;
  uint16_t temp;
  uint16_t battery;
  uint8_t if_frequency_hz_over_100;
  uint8_t if_mode;
  int8_t ppm=0;

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

  static bool audio_running;
  static void dma_handler();
  void process_block(uint16_t adc_samples[], int16_t audio[]);
  
  //store busy time for performance monitoring
  uint32_t busy_time;

  alarm_pool_t *pool = NULL;

  //volume control
  int16_t gain_numerator=0;

  //(optional) external oscillator
  quad_si5351 external_nco;
  bool external_nco_initialised = false;
  bool external_nco_good = false;
  bool external_nco_active = false;
  bool internal_nco_active = true;

  void pwm_ramp_down();
  void pwm_ramp_up();
    //Transmit
  button dit;
  button dah;
  uint8_t transmit_mode;
  void transmit();
  bool ptt();
  bool test_tone_enable;
  uint8_t test_tone_frequency;
  uint8_t tx_cw_paddle;
  uint8_t tx_cw_speed;
  uint8_t tx_mic_gain;
  bool tx_modulation;
  uint16_t tx_audio_level=0;
  uint8_t tx_pwm_min;
  uint8_t tx_pwm_max;
  uint8_t tx_pwm_threshold;
  
  // USB streaming mode
  uint8_t stream_raw_iq;

  public:
  rx(rx_settings & settings_to_apply, rx_status & status);
  void apply_settings();
  void run();
  void tune();
  void get_spectrum(uint8_t spectrum[], uint8_t &dB10, uint8_t zoom);
  void set_alarm_pool(alarm_pool_t *p);
  rx_settings &settings_to_apply;
  rx_status &status;
  rx_dsp rx_dsp_inst;
  void read_batt_temp();
  void access(bool settings_changed);
  void release();
  bool get_raw_data(int16_t &i, int16_t &q);
  uint32_t get_iq_buffer_level();
};
#endif
