#ifndef __ui_h__
#define __ui_h__

#include <cstdint>

#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "rx.h"
#include "memory.h"
#include "autosave_memory.h"
#include "waterfall.h"
#include "button.h"
#ifdef BUTTON_ENCODER
#include "button_encoder.h"
#else
#include "rotary_encoder.h"
#endif
#include "logo.h"
#include "u8g2.h"
#include "pins.h"
#include "settings.h"

// vscode cant find it and flags a problem (but the compiler can)
#ifndef M_PI
#define M_PI 3.14159265
#endif

// define wait macros
#define WAIT_10MS sleep_us(10000);
#define WAIT_100MS sleep_us(100000);

enum e_button_state {idle, slow_mode, fast_mode, very_fast_mode, menu, volume, mode};

// scanner
enum e_scanner_squelch {no_squelch, no_signal, signal_found, count_down};

// font styles styles as bits to be ORed
#define style_normal      0
#define style_reverse     (1<<0)
#define style_centered    (1<<1)
#define style_right       (1<<2)
#define style_nowrap      (1<<3)
#define style_bordered    (1<<4)
#define style_xor         (1<<5)

#define flag_enable_test_tone (0) //bit 0
#define mask_enable_test_tone (0x1 << flag_enable_test_tone)
#define flag_test_tone_frequency (1) //bits 1 - 5
#define mask_test_tone_frequency (0x1f << flag_test_tone_frequency)
#define flag_cw_paddle (6) //bit 6 - 7
#define mask_cw_paddle (0x3 << flag_cw_paddle)
#define flag_cw_speed (8) //bits 8 - 13
#define mask_cw_speed (0x3f << flag_cw_speed)
#define flag_tx_modulation (14) //bit 14
#define mask_tx_modulation (0x3f << flag_tx_modulation)
#define flag_mic_gain 15 // bits 15-18
#define mask_mic_gain (0xf << flag_mic_gain)
#define flag_pwm_threshold 19 //19-26
#define mask_pwm_threshold (0xff << flag_pwm_threshold)

class ui
{

  private:

  s_settings settings;
  const uint32_t timeout_lookup[8] = {0, 5000000, 10000000, 15000000, 30000000, 60000000, 120000000, 240000000};
  const char modes[6][4]  = {" AM", "AMS", "LSB", "USB", " FM", " CW"};
  const char smeter[13][12]  = {
    "S0",          "S1|",         "S2-|",        "S3--|",
    "S4---|",      "S5----|",     "S6-----|",    "S7------|",
    "S8-------|",  "S9--------|", "S9+10dB---|", "S9+20dB---|",
    "S9+30dB---|"};

  // Encoder
#ifdef BUTTON_ENCODER
  button_encoder main_encoder;
#else
  rotary_encoder main_encoder;
#endif

  // Buttons
  button menu_button;
  button back_button;
  button encoder_button;
  button dit;
  button dah;


  // Display
  void update_display_type(void);
  void setup_display();
  void display_clear(bool colour=0);

  void display_linen(uint8_t line);
  void display_set_xy(int16_t x, int16_t y);
  void display_add_xy(int16_t x, int16_t y);
  uint16_t display_get_x();
  uint16_t display_get_y();

  void display_print_char(char x, uint32_t scale=1, uint32_t style=0);
  void display_clear_str(uint32_t scale, bool colour=0);
  void display_print_str(const char str[], uint32_t scale=1, uint32_t style=0);
  void display_print_num(const char format[], int16_t num, uint32_t scale=1, uint32_t style=0);
  void display_print_freq(char separator, uint32_t frequency, uint32_t scale=1, uint32_t style=0);
  void display_print_speed(int16_t x, int16_t y, uint32_t scale, int speed);
  void display_draw_icon(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t pixels[]);
  void display_draw_volume(uint8_t v, uint8_t x);
  void display_draw_battery(float v, uint8_t x);

  void display_draw_separator(uint16_t y, uint32_t scale=1, bool colour=1);
  void display_show();
  int strchr_idx(const char str[], uint8_t c);
  bool do_splash();

  ssd1306_t disp;
  uint16_t cursor_x = 0;   // pixels 0-127
  uint16_t cursor_y = 0;   // pixels 0-63
  uint32_t display_time = 0;
  uint32_t display_timeout_max = 0;

  // Status                  
  float calculate_signal_strength(rx_status &status);

  void renderpage_transmit(rx_status & status, rx & receiver);

  void renderpage_original(rx_status & status, rx & receiver);
  void renderpage_bigspectrum(rx_status & status, rx & receiver);
  void renderpage_combinedspectrum(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_waterfall(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_status(rx_status & status, rx & receiver);
  void renderpage_fun(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_smeter(bool view_changed, rx_status & status, rx & receiver);

  int dBm_to_S(float power_dBm);
  float S_to_dBm(int S);
  int32_t dBm_to_63px(float power_dBm);
  void log_spectrum(float *min, float *max, int zoom = 1);
  void draw_h_tick_marks(uint16_t startY);
  void draw_spectrum(uint16_t startY, uint16_t endY);
  void draw_waterfall(uint16_t startY);
  void draw_slim_status(uint16_t y, rx_status & status, rx & receiver);
  void draw_vertical_dBm(uint16_t x, float power_dBm, float squelch);
  void draw_analogmeter(    uint16_t startx, uint16_t starty, 
                              uint16_t width, int16_t height,
                              float  needle_pct, int numticks = 0,
                              const char* legend = 0, const char labels[][5] = NULL
                              );
  bool frequency_scan(bool &ok);


  bool frequency_autosave_pending = false;
  uint8_t frequency_autosave_timer = 10u;

  // Menu                    
  bool main_menu(bool &ok);
  bool noise_menu(bool &ok);
  bool configuration_menu(bool &ok);
  bool bands_menu(bool &ok);
  bool spectrum_menu(bool &ok);

  //menu items
  void print_enum_option(const char options[], uint8_t option);
  void print_menu_option(const char options[], uint8_t option);

  bool menu_entry(const char title[], const char options[], uint32_t *value, bool &ok);
  bool enumerate_entry(const char title[], const char options[], uint8_t &value, bool &ok, bool &changed);
  bool bit_entry(const char title[], const char options[], bool &value, bool &ok);
  bool number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, int32_t &value, bool &ok, bool &changed);
  bool number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint8_t &value, bool &ok, bool &changed);
  bool number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, int8_t &value, bool &ok, bool &changed);
  bool frequency_entry(const char title[], uint32_t &which_setting, bool &ok);
  int string_entry(char string[], bool &ok, bool &del);
  bool memory_recall(bool &ok);
  bool memory_store(bool &ok);
  bool memory_scan(bool &ok);
  bool transmit_menu(bool &ok);
  bool upload_memory();
  void autosave();
  bool display_timeout(bool encoder_change);

  uint32_t regmode = 1;
  rx_settings &settings_to_apply;
  rx_status &status;
  rx &receiver;
  uint8_t *spectrum;
  uint8_t &dB10;
  uint8_t &zoom;
  waterfall &waterfall_inst;
  void apply_settings(bool suspend, bool settings_changed=true);

  u8g2_t u8g2;

  public:

  s_settings & get_settings(){return settings;};
  void autorestore();
  void do_ui();
  ui(rx_settings & settings_to_apply, rx_status & status, rx &receiver, uint8_t *spectrum, uint8_t &dB10, uint8_t &zoom, waterfall &waterfall_inst);
  void update_buttons(void);
  void update_paddles(void);

};

#endif
