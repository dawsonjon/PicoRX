#ifndef RX_DSP_H
#define RX_DSP_H

#include <stdint.h>
#include "rx_definitions.h"
#include "half_band_filter.h"
#include "half_band_filter2.h"

class rx_dsp
{
  public:

  rx_dsp();
  uint16_t process_block(uint16_t samples[], int16_t audio_samples[]);
  void set_frequency_offset_Hz(double offset_frequency);
  void set_agc_speed(uint8_t agc_setting);
  void set_mode(uint8_t mode);
  void set_cw_sidetone_Hz(uint16_t val);
  int32_t get_signal_amplitude();
  void get_spectrum(int16_t spectrum[], int16_t &offset);


  private:
  
  void frequency_shift(int16_t &i, int16_t &q);
  bool decimate(int16_t &i, int16_t &q);
  int16_t demodulate(int16_t i, int16_t q);
  int16_t automatic_gain_control(int16_t audio);
  bool cw_decimate(int16_t &i, int16_t &q);

  //capture samples for spectral analysis
  int16_t capture_i[256];
  int16_t capture_q[256];

  //used in dc canceler
  int32_t dc;

  //used in frequency shifter
  uint32_t offset_frequency_Hz;
  uint32_t phase;
  uint32_t frequency;

  //used in CIC filter
  uint8_t decimate_count;
  int32_t integratori1, integratorq1;
  int32_t integratori2, integratorq2;
  int32_t integratori3, integratorq3;
  int32_t integratori4, integratorq4;
  int32_t delayi0, delayq0;
  int32_t delayi1, delayq1;
  int32_t delayi2, delayq2;
  int32_t delayi3, delayq3;

  //used in half band filter
  half_band_filter half_band_filter_inst;
  half_band_filter2 half_band_filter2_inst;

  //used in CW CIC filter
  uint8_t cw_decimate_count;
  int32_t cw_integratori1, cw_integratorq1;
  int32_t cw_integratori2, cw_integratorq2;
  int32_t cw_integratori3, cw_integratorq3;
  int32_t cw_integratori4, cw_integratorq4;
  int32_t cw_delayi0, cw_delayq0;
  int32_t cw_delayi1, cw_delayq1;
  int32_t cw_delayi2, cw_delayq2;
  int32_t cw_delayi3, cw_delayq3;

  //used in CW half band filter
  half_band_filter cw_half_band_filter_inst;
  half_band_filter2 cw_half_band_filter2_inst;

  //used to generate cw sidetone
  int16_t cw_magnitude;
  int16_t cw_sidetone_phase;
  int16_t cw_sidetone_frequency_Hz=1000;

  int32_t signal_amplitude;

  //used in demodulator
  half_band_filter2 ssb_filter;
  int32_t mode=0;
  int32_t audio_dc=0;
  uint8_t ssb_phase=0;
  int16_t audio_phase=0;
  int16_t last_audio_phase=0;

  //test tone
  int16_t signal = 0;

  //used in AGC
  uint8_t attack_factor;
  uint8_t decay_factor;
  uint16_t hang_time;
  uint16_t hang_timer;
  const bool agc_enabled = true;
  int32_t max_hold;

};

#endif
