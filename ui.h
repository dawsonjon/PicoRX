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
#include "event.h"

const uint8_t PIN_AB = 20;
const uint8_t PIN_B  = 21;
const uint8_t PIN_MENU = 22;
const uint8_t PIN_BACK = 17;
const uint8_t PIN_ENCODER_PUSH = 5;
const uint8_t PIN_DISPLAY_SDA = 18;
const uint8_t PIN_DISPLAY_SCL = 19;

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

// bit flags for HW settings in idx_hw_setup
#define flag_reverse_encoder 0
#define flag_swap_iq 1
#define flag_flip_oled 2
#define flag_oled_type 3
#define flag_display_timeout 4  // bits 4-6
  #define mask_display_timeout (0x7 << flag_display_timeout)
#define flag_display_contrast 7   // bits 7-10 stored ^0xf so 0000 is full bright
  #define mask_display_contrast (0xf << flag_display_contrast)
#define flag_encoder_res 11
// #define flag_nextfree 12
// 32bit number, 31 is last flag bit

//flags for receiver features idx_rx_features
#define flag_enable_auto_notch (0)
#define flag_deemphasis (1)
  #define mask_deemphasis (0x3 << flag_deemphasis)

// define wait macros
#define WAIT_10MS sleep_us(10000);
#define WAIT_100MS sleep_us(100000);

enum e_button_state {idle, down, slow_mode, fast_mode, very_fast_mode, menu};

// scanner
enum e_scanner_squelch {no_signal, signal_found, count_down};

// font styles styles as bits to be ORed
#define style_normal      0
#define style_reverse     (1<<0)
#define style_centered    (1<<1)
#define style_right       (1<<2)
#define style_nowrap      (1<<3)
#define style_bordered    (1<<4)
#define style_xor         (1<<5)

class ui
{

  private:

  uint32_t settings[16];
  float spectrum[128];
  const uint32_t step_sizes[11] = {10, 50, 100, 1000, 5000, 9000, 10000, 12500, 25000, 50000, 100000};
  const uint16_t timeout_lookup[8] = {0, 50, 100, 150, 300, 600, 1200, 2400};
  const char modes[6][4]  = {" AM", "AMS", "LSB", "USB", " FM", " CW"};
  const char steps[11][8]  = {
    "10Hz", "50Hz", "100Hz", "1kHz",
    "5kHz", "9kHz", "10kHz", "12.5kHz", "25kHz",
    "50kHz", "100kHz"};
  const char smeter[13][12]  = {
    "S0",          "S1|",         "S2-|",        "S3--|",
    "S4---|",      "S5----|",     "S6-----|",    "S7------|",
    "S8-------|",  "S9--------|", "S9+10dB---|", "S9+20dB---|",
    "S9+30dB---|"};
  // last selected memory
  int32_t last_select=0;

  // Encoder 
  void setup_encoder();
  int32_t get_encoder_change();
  int32_t encoder_control(int32_t *value, int32_t min, int32_t max);
  int32_t new_position;
  int32_t old_position;
  const uint32_t sm = 0;
  const PIO pio = pio1;

  // Buttons
  e_button_state button_state;
  uint8_t timeout;

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
  void display_print_speed(int16_t x, int16_t y, uint32_t scale, int speed);
  void display_draw_icon7x8(uint8_t x, uint8_t y, const uint8_t (&data)[7]);
  void display_draw_volume(uint8_t v);

  void display_draw_separator(uint16_t y, uint32_t scale=1, bool colour=1);
  void display_show();
  int strchr_idx(const char str[], uint8_t c);
  bool do_splash();

  ssd1306_t disp;
  int16_t cursor_x = 0;   // pixels 0-127
  int16_t cursor_y = 0;   // pixels 0-63
  uint16_t display_timer = 0;

  // Status                  
  float calculate_signal_strength(rx_status &status);

  void renderpage_original(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_bigspectrum(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_waterfall(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_bigtext(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_smeter(bool view_changed, rx_status & status, rx & receiver);
  void renderpage_fun(bool view_changed, rx_status & status, rx & receiver);
  #define NUM_VIEWS 6

  int dBm_to_S(float power_dBm);
  float S_to_dBm(int S);
  int32_t dBm_to_63px(float power_dBm);
  void log_spectrum(float *min, float *max);
  void draw_h_tick_marks(uint16_t startY);
  void draw_spectrum(uint16_t startY, rx & receiver);
  void draw_waterfall(uint16_t startY, rx & receiver);
  void draw_slim_status(uint16_t y, rx_status & status, rx & receiver);
  void draw_vertical_dBm(uint16_t x, float power_dBm, float squelch);
  void draw_analogmeter(    uint16_t startx, uint16_t starty, 
                              uint16_t width, int16_t height,
                              float  needle_pct, int numticks = 0,
                              const char* legend = 0, const char labels[][5] = NULL
                              );


  bool frequency_autosave_pending = false;
  uint8_t frequency_autosave_timer = 10u;

  // Menu                    
  void print_enum_option(const char options[], uint8_t option);
  void print_menu_option(const char options[], uint8_t option);
  uint32_t menu_entry(const char title[], const char options[], uint32_t *value);
  uint32_t enumerate_entry(const char title[], const char options[], uint32_t *value);
  uint32_t bit_entry(const char title[], const char options[], uint8_t bit_position, uint32_t *value);
  int16_t number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint32_t *value);
  bool frequency_entry(const char title[], uint32_t which_setting);
  int string_entry(char string[]);
  bool top_menu(rx_settings & settings_to_apply);
  bool configuration_menu();
  bool scanner_menu();
  bool scanner_radio_menu();
  bool frequency_scan();
  bool memory_recall();
  bool memory_scan();
  bool memory_store();
  int get_memory_name(char* name, int select, bool strip_spaces);

  bool upload_memory();
  void autosave();
  void apply_settings(bool suspend);
  bool display_timeout(bool encoder_change, event_t event);

  uint32_t regmode = 1;

  rx_settings &settings_to_apply;
  rx_status &status;
  rx &receiver;

  public:

  void autorestore();
  void do_ui(event_t event);
  ui(rx_settings & settings_to_apply, rx_status & status, rx &receiver);

};

#include "logo.h"
#endif
