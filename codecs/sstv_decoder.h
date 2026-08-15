//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: sstv_decoder.h
// description: class to decode sstv from audio
// License: MIT
//

#ifndef __SSTV_DECODER_H__
#define __SSTV_DECODER_H__

#include <cstdint>

enum e_sstv_mode
{
  martin_m1,
  martin_m2,
  scottie_s1,
  scottie_s2,
  scottie_dx,
  pd_50,
  pd_90,
  pd_120,
  pd_180,
  sc2_60,
  sc2_120,
  sc2_180,
  robot24,
  robot36,
  robot72,
  bw8,
  bw12,
  num_modes
};

enum e_sync_state
{
  detect,
  confirm,
};

enum e_sstv_state
{
  detect_sync,
  confirm_sync,
  decode_line,
};

struct s_sstv_mode
{
  uint16_t width;
  uint16_t max_height;
  uint32_t samples_per_line;
  uint32_t samples_per_colour_line;
  uint32_t samples_per_colour_gap;
  uint32_t samples_per_pixel;
  uint32_t samples_per_hsync;
  char const* mode_string;
};

class c_sstv_decoder
{

private:
  s_sstv_mode sstv_mode;
  float m_Fs;
  uint32_t m_scale;
  uint32_t sync_counter = 0;
  uint16_t last_x = 0;
  uint16_t last_y = 0;
  uint32_t m_image_sample = 0;
  uint16_t last_sample = 0;
  uint32_t last_hsync_sample = 0;
  uint32_t sample_number = 0;
  uint32_t confirmed_sync_sample = 0;
  e_sstv_state state = detect_sync;
  e_sync_state sync_state = detect;
  void sample_to_pixel(uint16_t& x, uint16_t& y, uint8_t& colour, int32_t image_sample);
  uint8_t frequency_to_brightness(uint16_t x);
  uint32_t mean_samples_per_line;
  uint32_t sync_timeout = 0;
  uint32_t confirm_count;
  uint32_t pixel_accumulator;
  uint16_t pixel_n;
  int16_t last_phase = 0;
  uint8_t ssb_phase = 0;
  int16_t frequency;
  e_sstv_mode decode_mode;
  s_sstv_mode modes[num_modes];
  bool m_auto_slant_correction;
  uint32_t m_timeout;
  bool m_image_open_flag = false;
  bool m_image_complete_flag = false;
  uint8_t m_line[640][5]; // array to contain seperate colour components of each decoded line

  void decode_sample(uint16_t sample, uint16_t& pixel_y, uint16_t& pixel_x, uint8_t& pixel_colour,
                     uint8_t& pixel, bool& pixel_complete, bool& line_complete,
                     bool& image_complete);

  // override one of these hardware dependent functions.
  /////////////////////////////////////////////////////////////////////////////

  // The decoder can work with frequency data, IQ data or (real, mono) audio samples.
  // override one of these deneding on what you need.

  virtual int16_t get_audio_sample() = 0;
  virtual void get_iq_sample(int16_t& i, int16_t& q)
  {
    (void)i;
    (void)q;
  };
  virtual uint16_t get_frequency_sample();

  // Override this function to output a line of image
  virtual void image_write_line(uint16_t line_rgb565[], uint16_t y, uint16_t width, uint16_t height,
                                const char* mode_string) = 0;
  virtual void scope(uint16_t mag, int16_t freq)
  {
    (void)mag;
    (void)freq;
  };

public:
  c_sstv_decoder(float Fs);
  void decode_image(uint8_t timeout_s, bool slant_correction);
  bool decode_image_non_blocking(uint8_t timeout_s, bool slant_correction, bool& image_in_progress);
};

#endif
