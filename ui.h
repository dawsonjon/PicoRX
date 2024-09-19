#ifndef __ui_h__
#define __ui_h__

#include <cstdint>

#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "quadrature_encoder.pio.h"
#include "ssd1306.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "rx.h"
#include "memory.h"
#include "autosave_memory.h"
#include "waterfall.h"
#include "button.h"

const uint8_t PIN_AB = 20;
const uint8_t PIN_B  = 21;
const uint8_t PIN_MENU = 22;
const uint8_t PIN_BACK = 17;
const uint8_t PIN_ENCODER_PUSH = 5;
const uint8_t PIN_DISPLAY_SDA = 18;
const uint8_t PIN_DISPLAY_SCL = 19;

#define MODE_AM 0
#define MODE_AMS 1
#define MODE_LSB 2
#define MODE_USB 3
#define MODE_FM 4
#define MODE_CW 5

// settings that get stored in eeprom
#define settings_to_store 6
#define idx_frequency 0
#define idx_mode 1
#define idx_agc_speed 2
#define idx_step 3
#define idx_max_frequency 4
#define idx_min_frequency 5
#define idx_squelch 6
#define idx_volume 7
#define idx_cw_sidetone 8
#define idx_hw_setup 9
#define idx_gain_cal 10
#define idx_bandwidth 11
#define idx_rx_features 12
#define idx_oled_contrast 13  // values 0..15 get * 17 for set_contrast(0.255)

// bit flags for HW settings in idx_hw_setup
#define flag_reverse_encoder 0
#define flag_swap_iq 1
#define flag_flip_oled 2
#define flag_oled_type 3
#define flag_display_timeout 4
#define mask_display_timeout (0x7 << flag_display_timeout)

//flags for receiver features idx_rx_features
#define flag_enable_auto_notch 0

// define wait macros
#define WAIT_10MS sleep_us(10000);
#define WAIT_100MS sleep_us(100000);

enum e_button_state {idle, down, slow_mode, fast_mode, very_fast_mode, menu};

// font styles styles as bits to be ORed
#define style_normal      0
#define style_reverse     (1<<0)
#define style_centered    (1<<1)
#define style_right       (1<<2)
#define style_nowrap      (1<<3)
#define style_bordered    (1<<4)

const uint32_t step_sizes[10] = {10, 50, 100, 1000, 5000, 10000, 12500, 25000, 50000, 100000};

class ui
{

  private:

  uint32_t settings[16];
  const uint32_t timeout_lookup[8] = {0, 5000000, 10000000, 15000000, 30000000, 60000000, 120000000, 240000000};
  const char modes[6][4]  = {" AM", "AMS", "LSB", "USB", " FM", " CW"};
  const char steps[10][8]  = {
    "10Hz", "50Hz", "100Hz", "1kHz",
    "5kHz", "10kHz", "12.5kHz", "25kHz",
    "50kHz", "100kHz"};
  const char smeter[13][12]  = {
    "S0",          "S1|",         "S2-|",        "S3--|",
    "S4---|",      "S5----|",     "S6-----|",    "S7------|",
    "S8-------|",  "S9--------|", "S9+10dB---|", "S9+20dB---|",
    "S9+30dB---|"};

  // Encoder 
  void setup_encoder();
  int32_t get_encoder_change();
  int32_t encoder_control(int32_t *value, int32_t min, int32_t max);
  int32_t new_position;
  int32_t old_position;
  const uint32_t sm = 0;
  const PIO pio = pio1;

  // Buttons
  button menu_button;
  button back_button;
  button encoder_button;

  // Display
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

  void renderpage_original(rx_status & status, rx & receiver);
  void renderpage_bigspectrum(rx_status & status, rx & receiver);
  void renderpage_waterfall(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_bigtext(rx_status & status, rx & receiver);
  void renderpage_fun(rx_status & status, rx & receiver);
  void renderpage_smeter(bool view_changed, rx_status & status, rx & receiver);

  int dBm_to_S(float power_dBm);
  void log_spectrum(float *min, float *max);
  void draw_h_tick_marks(uint16_t startY);
  void draw_spectrum(uint16_t startY);
  void draw_waterfall(uint16_t startY);
  void draw_slim_status(uint16_t y, rx_status & status, rx & receiver);
  void draw_analogmeter(    uint16_t startx, uint16_t starty, 
                              int16_t width, int16_t height,
                              float  needle_pct, int numticks = 0,
                              const char* legend = 0, const char labels[][5] = NULL
                              );


  bool frequency_autosave_pending = false;
  uint8_t frequency_autosave_timer = 10u;

  // Menu                    
  bool main_menu(bool &ok);
  bool configuration_menu(bool &ok);

  //menu items
  void print_enum_option(const char options[], uint8_t option);
  void print_menu_option(const char options[], uint8_t option);
  bool menu_entry(const char title[], const char options[], uint32_t *value, bool &ok);
  bool enumerate_entry(const char title[], const char options[], uint32_t *value, bool &ok);
  bool bit_entry(const char title[], const char options[], uint8_t bit_position, uint32_t *value, bool &ok);
  bool number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint32_t *value, bool &ok);
  bool frequency_entry(const char title[], uint32_t which_setting, bool &ok);
  int string_entry(char string[], bool &ok, bool &del);
  bool memory_recall(bool &ok);
  bool memory_store(bool &ok);
  bool upload_memory();
  void autosave();
  void apply_settings(bool suspend);
  bool display_timeout(bool encoder_change);


  uint32_t regmode = 1;

  rx_settings &settings_to_apply;
  rx_status &status;
  rx &receiver;
  uint8_t *spectrum;
  uint8_t &dB10;

  public:

  uint32_t * get_settings(){return &settings[0];};
  void autorestore();
  void do_ui();
  ui(rx_settings & settings_to_apply, rx_status & status, rx &receiver, uint8_t *spectrum, uint8_t &dB10);

};

#include "logo.h"
#endif
