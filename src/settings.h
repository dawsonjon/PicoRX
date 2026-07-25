#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "xcvr.h"

const uint32_t step_sizes[13] = {10, 50, 100, 500, 1000, 5000, 6250, 9000, 10000, 12500, 25000, 50000, 100000};
const char steps[13][8]  = { "10Hz", "50Hz", "100Hz", "500Hz", "1kHz", "5kHz", "6.25kHz", "9kHz", "10kHz", "12.5kHz", "25kHz", "50kHz", "100kHz"};

const uint8_t  autosave_chan_size = 32;
const uint8_t  memory_chan_size = 16;
const uint16_t num_chans = 512;

enum e_mode
{
  MODE_AM = 0,
  MODE_AMS = 1,
  MODE_LSB = 2,
  MODE_USB = 3,
  MODE_FM = 4,
  MODE_CW = 5,
};

//*** NOTE TO SELF ***
//*****************************************************************************
//remember to change channel_to_words.py
//*****************************************************************************

struct s_channel_settings
{
  uint32_t frequency;
  uint32_t max_frequency;
  uint32_t min_frequency;
  uint8_t  mode;
  uint8_t  agc_setting;
  uint8_t  step;
  uint8_t  bandwidth;
};

struct s_global_settings
{
  s_tx_band_limits tx_band_limits;
  uint8_t volume;
  uint8_t cw_sidetone;
  uint8_t squelch_threshold;
  uint8_t squelch_timeout;
  uint8_t spectrum_zoom;
  uint8_t deemphasis;
  uint8_t regmode;
  uint8_t display_timeout;
  uint8_t display_contrast;
  uint8_t tft_rotation;
  uint8_t tft_colour;
  uint8_t tft_driver;
  uint8_t tft_invert;
  uint8_t gain_cal;
  int8_t  ppm;
  uint8_t band1;
  uint8_t band2;
  uint8_t band3;
  uint8_t band4;
  uint8_t band5;
  uint8_t band6;
  uint8_t band7;
  uint8_t tx_phase_dither;
  int8_t tx_i_offset;
  int8_t tx_q_offset;
  int8_t tx_iq_balance;
  uint8_t pwm_min;
  uint8_t pwm_max;
  uint8_t test_tone_frequency;
  uint8_t test_tone_setting;
  uint8_t cw_paddle;
  uint8_t cw_speed;
  uint8_t mic_gain;
  uint8_t pwm_threshold;
  uint8_t tx_compression;
  int8_t tx_treble;
  int8_t tx_bass;
  uint8_t if_frequency_hz_over_100;
  uint8_t if_mode;
  uint8_t noise_estimation;
  uint8_t noise_threshold;
  uint8_t view;
  uint8_t spectrum_smoothing;
  uint8_t hold_smoothing;
  uint8_t treble;
  uint8_t bass;
  uint8_t aux_view;
  uint8_t impulse_threshold;
  uint8_t agc_gain;
  bool    usb_stream;
  bool    enable_auto_notch;
  bool    iq_correction;
  bool    enable_noise_reduction;
  bool    reverse_encoder;
  bool    encoder_resolution;
  bool    swap_iq;
  bool    flip_oled;
  bool    oled_type;
  bool    tx_modulation;
  bool    tx_use_best_clock;
  bool    tx_monitor;
  bool    tx_noise_gate;
  bool    enable_external_nco;
  bool    spectrum_hold;
};

struct s_settings
{
  s_channel_settings channel;
  s_global_settings global;
};

const s_settings default_settings = {
{
  1413000,  //frequency
  30000000, //max_frequency
  0,        //min_frequency
  0,        //mode = AM
  3,        //agc_setting = very_slow
  4,        //step = 1kHz
  2,        //bandwidth = normal
}, {
  {
    //TX Bands Lower
    {
      36,  //*0.05 = 1.8   (160m)
      70,  //*0.05 = 3.5   (80m)
      105, //*0.05 = 5.25  (60m)
      140, //*0.05 = 7     (40m)
      202, //*0.05 = 10.1  (30m)
      280, //*0.05 = 14    (20m)
      361, //*0.05 = 18.05 (17m)
      420, //*0.05 = 21    (15m)
    },
    //TX Bands Upper
    {
      40,  //*0.05 = 2     (160m)
      76,  //*0.05 = 3.8   (80m)
      109, //*0.05 = 5.45  (60m)
      144, //*0.05 = 7.2   (40m)
      203, //*0.05 = 10.15 (30m)
      287, //*0.05 = 14.35 (20m)
      364, //*0.05 = 18.2  (17m)
      429, //*0.05 = 21.45 (15m)
    },
  },
  5,  //volume
  10, //cw_sidetone = 1000Hz
  0,  //squelch_threshold
  0,  //squelch_timeout = never
  1,  //spectrum_zoom
  0,  //deemphasis
  0,  //regmode
  0,  //display_timeout = never
  15, //display_contrast = 255
  5,  //tft_rotation
  1,  //tft_colour
  0,  //tft_invert
  0,  //tft_driver
  62, //gain_cal
  0,  //ppm
  0x02, //band1
  0x04, //band2
  0x08, //band3
  0x10, //band4
  0x20, //band5
  0x40, //band6
  0x80, //band7
  32,   //phase dither
  -8,    //tx_i_offset
  -8,   //tx_q_offset
  0,    //tx_iq_balance
  0x00, //pwm_min
  0xff, //pwm_max
  10,   //test_tone_frequency
  0,    //test_tone_setting
  0,    //cw_paddle;
  12,   //cw_speed;
  4,    //mic_gain;
  0,    //pwm_threshold;
  0,    //tx_compression;
  3,    //tx_treble;
  -3,   //tx_bass;
  45, //if_frequency_hz_over_100 = 4500Hz
  2,  //if_mode = nearest
  2,  //noise_estimation very_fast, fast, normal, slow, very_slow
  0,  //noise_threshold normal, high, very_high
  0,  //view
  4,  //spectrum_smoothing
  6,  //hold_smoothing
  0,  //treble
  0,  //bass
  0,  //aux_view
  0,  //impulse blanker threshold
  6,  //agc_gain
  0,  //usb_stream
  0,  //enable_auto_notch
  0,  //iq_correction
  0,  //enable_noise_reduction
  0,  //reverse_encoder
  0,  //encoder_resolution
  0,  //swap_iq
  0,  //flip_oled
  0,  //oled_type = ssd1306
  1,  //tx_modulation
  1,  //tx_use_best_clock
  0,  //tx_monitor
  1,  //tx_noise_gate
  0,  //enable_external_nco
  1,  //spectrum_hold
}};


struct s_memory_channel
{
  s_channel_settings channel;
  char label[17]; //16 characters + null terminator
};


void apply_settings_to_xcvr(xcvr & transceiver, xcvr_settings & xcvr_settings, s_settings & settings, bool suspend, bool settings_changed);
void autosave_restore_settings(s_settings &settings);
void autosave_store_settings(s_settings settings, xcvr & transceiver, xcvr_settings & xcvr_settings);
s_memory_channel get_channel(uint16_t channel_number);
void memory_store_channel(s_memory_channel memory_channel, uint16_t channel_number, s_settings & settings, xcvr & transceiver, xcvr_settings & xcvr_settings);

#endif
