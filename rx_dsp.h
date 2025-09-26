#ifndef RX_DSP_H
#define RX_DSP_H

#include <stdint.h>
#include "rx_definitions.h"
#include "pico/sem.h"
#include "pico/util/queue.h"
#include "fft_filter.h"
#include "ring_buffer_lib.h"

typedef struct {
  int32_t phase_locked;
  int32_t x1;
  int32_t x2;
  int32_t y1;
  int32_t y2;
  int32_t y0_err;
} amsync_t;

class rx_dsp
{
  public:

  rx_dsp();
  uint16_t process_block(uint16_t samples[], int16_t audio_samples[], ring_buffer_t *iq_samples);
  void set_frequency_offset_Hz(double offset_frequency);
  void set_agc_control(uint8_t agc_control, uint8_t agc_gain);
  void set_mode(uint8_t mode, uint8_t bw);
  void set_cw_sidetone_Hz(uint16_t val);
  void set_gain_cal_dB(uint16_t val);
  void set_squelch(uint8_t threshold, uint8_t timeout);
  void set_swap_iq(uint8_t val);
  void set_iq_correction(uint8_t val);
  void set_deemphasis(uint8_t deemph);
  void set_treble(uint8_t tr);
  void set_bass(uint8_t bs);
  void set_impulse_threshold(uint8_t it);
  void set_auto_notch(bool enable_auto_notch);
  void set_noise_reduction(bool enable_noise_reduction, int8_t noise_smoothing, int8_t noise_threshold);
  void set_spectrum_smoothing(uint8_t spectrum_smoothing);
  int16_t get_signal_strength_dBm();
  void get_spectrum(uint8_t spectrum[], uint8_t &dB10, uint8_t zoom);
  void get_audio_capture(uint8_t audio[]);
  s_filter_control get_filter_config();
  void get_spectrum(float spectrum[]);
  bool get_raw_data(int16_t &i, int16_t &q);
  uint32_t get_iq_buffer_level();
  float get_tuning_offset_Hz();
  void amsync_reset(void);

  private:
  
  void frequency_shift(int16_t &i, int16_t &q);
  bool decimate(int16_t &i, int16_t &q);
  int16_t demodulate(int16_t i, int16_t q, uint16_t mag, int16_t phi);
  int16_t automatic_gain_control(int16_t audio);
  int16_t apply_deemphasis(int16_t x);
  int16_t squelch(int16_t audio, int32_t amplitude);
  int16_t apply_treble(int16_t x);
  int16_t apply_bass(int16_t x);
  void apply_impulse_blanker(int16_t &i, int16_t &q, uint16_t mag);
  void iq_imbalance_correction(int16_t &i, int16_t &q);

  //capture samples for decoding
  queue_t data_queue;

  //capture samples for spectral analysis
  int16_t capture[256];
  semaphore_t spectrum_semaphore;

  //capture samples for waveform display
  int16_t audio_capture[128];
  uint16_t audio_capture_idx;
  semaphore_t audio_semaphore;

  //used in cic decimator
  uint8_t decimate_count;
  int32_t integratori1, integratorq1;
  int32_t integratori2, integratorq2;
  int32_t integratori3, integratorq3;
  int32_t integratori4, integratorq4;
  int32_t delayi0, delayq0;
  int32_t delayi1, delayq1;
  int32_t delayi2, delayq2;
  int32_t delayi3, delayq3;

  //used in fft filter
  int16_t fft_bin;
  fft_filter fft_filter_inst;
  s_filter_control filter_control;
  s_filter_control capture_filter_control;

  //used in frequency shifter
  uint8_t swap_iq;
  uint8_t iq_correction;
  int32_t offset_frequency_Hz;
  int32_t dither;
  uint32_t phase;
  int32_t frequency;
  int64_t frequency_accumulator = 0;
  int32_t frequency_count = 0;
  float frequency_offset_Hz = 0.0f;

  //used to generate cw sidetone
  int16_t cw_i, cw_q;
  int16_t cw_sidetone_phase;
  int16_t cw_sidetone_frequency_Hz=1000;

  int32_t signal_amplitude;

  //used in demodulator
  int32_t mode=0;
  int32_t audio_dc=0;
  uint8_t ssb_phase=0;
  int16_t last_phase=0;

  // de-emphasis
  uint8_t deemphasis=0;

  // treble
  uint8_t treble = 0;

  //bass
  uint8_t bass = 0;

  // impulse blanker threshold
  uint8_t impulse_threshold;

  //squelch
  int16_t squelch_threshold=0;
  int16_t s9_threshold=0;
  uint32_t squelch_time_ms = 0;
  uint32_t squelch_timeout_ms = 0;

  //used in AGC
  uint8_t attack_factor;
  uint8_t decay_factor;
  uint16_t hang_time;
  uint16_t hang_timer;
  int32_t max_hold;
  int16_t gain;
  int16_t manual_gain;
  bool manual_gain_control = false;

  // gain calibration
  float amplifier_gain_dB = 62.0f;
  int32_t usb_buf_level_avg = 0;

  // synchronous AM demodulator state
  amsync_t amsync;

};

#endif
