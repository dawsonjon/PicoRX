#ifndef __ui_h__
#define __ui_h__

#include <cstdint>

#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "quadrature_encoder.pio.h"
#include "ssd1306.h"
#include "font.h"
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

//flags for HW settings
#define flag_reverse_encoder 0
#define flag_swap_iq 1
#define flag_flip_oled 2
#define flag_oled_type 3
#define flag_display_timeout 4
#define mask_display_timeout (0x7 << flag_display_timeout)

//flags for receiver features
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
//#define style_bold        (1<<2)


class ui
{

  private:

  uint32_t settings[16];
  const uint32_t step_sizes[10] = {10, 50, 100, 1000, 5000, 10000, 12500, 25000, 50000, 100000};
  const uint16_t timeout_lookup[8] = {0, 50, 100, 150, 300, 600, 1200, 2400};
  const char modes[5][4]  = {" AM", "LSB", "USB", " FM", " CW"};
  const char steps[10][8]  = {
    "10Hz", "50Hz", "100Hz", "1kHz",
    "5kHz", "10kHz", "12.5kHz", "25kHz",
    "50kHz", "100kHz"};
  const char smeter[13][12]  = {
    "S0",          "S1|",         "S2-|",        "S3--|",
    "S4---|",      "S5----|",     "S6-----|",    "S7------|",
    "S8-------| ", "S9--------|", "S9+10dB---|", "S9+20dB---|",
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
  void display_set_xy(uint16_t x, uint16_t y);
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

  ssd1306_t disp;
  uint16_t cursor_x = 0;   // pixels 0-127
  uint16_t cursor_y = 0;   // pixels 0-63
  uint16_t display_timer = 0;

  // Status                  
  float calculate_signal_strength(rx_status &status);

  void update_display(rx_status & status, rx & receiver);
  void update_display2(rx_status & status, rx & receiver);
  void update_display3(rx_status & status, rx & receiver);
  #define NUM_VIEWS 3

  void draw_spectrum(rx & receiver, uint16_t startY);
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
  bool configuration_menu();
  bool recall();
  bool store();
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
