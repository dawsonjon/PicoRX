#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "rx.h"

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
  uint8_t  agc_gain;
  uint8_t  step;
  uint8_t  bandwidth;
};

struct s_global_settings
{
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
  uint8_t pwm_min;
  uint8_t pwm_max;
  uint8_t test_tone_frequency;
  uint8_t cw_paddle;
  uint8_t cw_speed;
  uint8_t mic_gain;
  uint8_t pwm_threshold;
  uint8_t if_frequency_hz_over_100;
  uint8_t if_mode;
  uint8_t noise_estimation;
  uint8_t noise_threshold;
  uint8_t view;
  uint8_t spectrum_smoothing;
  uint8_t treble;
  uint8_t bass;
  uint8_t aux_view;
  bool    usb_stream;
  bool    enable_auto_notch;
  bool    iq_correction;
  bool    enable_noise_reduction;
  bool    reverse_encoder;
  bool    encoder_resolution;
  bool    swap_iq;
  bool    flip_oled;
  bool    oled_type;
  bool    test_tone_enable;
  bool    tx_modulation;
  bool    enable_external_nco;

};

struct s_settings
{
  s_channel_settings channel;
  s_global_settings global;
};

const s_settings default_settings = {
{
  7074000,  //frequency
  30000000, //max_frequency
  0,        //min_frequency
  2,        //mode = LSB
  3,        //agc_setting = very_slow
  10,       //agc_gain
  4,        //step = 1kHz
  2,        //bandwidth = normal
}, {
  5,  //volume
  7, //cw_sidetone = 1000Hz
  0,  //squelch_threshold
  0,  //squelch_timeout = never
  2,  //spectrum_zoom
  0,  //deemphasis
  0,  //regmode
  0,  //display_timeout = never
  15, //display_contrast = 255
  6,  //tft_rotation
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
  0x00, //pwm_min
  0x55, //pwm_max
  10,   //test_tone_frequency
  1,    //cw_paddle;
  0,    //cw_speed;
  0,    //mic_gain;
  0,    //pwm_threshold;
  45, //if_frequency_hz_over_100 = 4500Hz
  2,  //if_mode = nearest
  2,  //noise_estimation very_fast, fast, normal, slow, very_slow
  0,  //noise_threshold normal, high, very_high
  0,  //view
  1,  //spectrum_smoothing
  0,  //treble
  0,  //bass
  0,  //aux_view
  0,  //usb_stream
  0,  //enable_auto_notch
  0,  //iq_correction
  0,  //enable_noise_reduction
  0,  //reverse_encoder
  0,  //encoder_resolution
  0,  //swap_iq
  0,  //flip_oled
  0,  //oled_type = ssd1306
  0,  //enable_test_tone
  0,  //tx_modulation
  1,  //enable_external_nco
}};


struct s_memory_channel
{
  s_channel_settings channel;
  char label[17]; //16 characters + null terminator
};


void apply_settings_to_rx(rx & receiver, rx_settings & rx_settings, s_settings & settings, bool suspend, bool settings_changed);
void autosave_restore_settings(s_settings &settings);
void autosave_store_settings(s_settings settings, rx & receiver, rx_settings & rx_settings);
s_memory_channel get_channel(uint16_t channel_number);
void memory_store_channel(s_memory_channel memory_channel, uint16_t channel_number, s_settings & settings, rx & receiver, rx_settings & rx_settings);

#endif
