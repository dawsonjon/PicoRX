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

#define flag_reverse_encoder 0
#define flag_swap_iq 1
#define flag_flip_oled 2
#define flag_oled_type 3

#define idx_gain_cal 10

// define wait macros
#define WAIT_10MS sleep_us(10000);
#define WAIT_100MS sleep_us(100000);
#define WAIT_500MS sleep_us(500000);;

enum e_button_state {idle, down, fast_mode, menu};

// font styles styles as bits to be ORed
#define style_normal      0
#define style_reverse     (1<<0)
#define style_centered    (1<<1)
//#define style_bold        (1<<2)


class ui
{

  private:

  uint32_t settings[16];
  const uint32_t step_sizes[10] = {10, 50, 100, 1000, 5000, 10000, 12500, 25000, 50000, 100000};

  // Encoder 
  void setup_encoder();
  int32_t get_encoder_change();
  int32_t encoder_control(int32_t *value, int32_t min, int32_t max);
  int32_t new_position;
  int32_t old_position;
  const uint32_t sm = 0;
  const PIO pio = pio1;

  // Buttons 
  void setup_buttons();
  bool get_button(uint8_t button);
  bool check_button(unsigned button);
  e_button_state button_state;
  uint8_t timeout;

  // Display
  void setup_display();
  void display_clear(bool colour=0);
  void display_line1();
  void display_line2();
  void display_linen(uint8_t line);
  void display_set_xy(uint8_t x, uint8_t y);
  void display_print_char(char x, uint32_t scale=1, uint32_t style=0);
  void display_print_str(const char str[], uint32_t scale=1, uint32_t style=0);
  void display_print_num(const char format[], int16_t num, uint32_t scale=1, uint32_t style=0);
  void display_show();

  ssd1306_t disp;
  uint8_t cursor_x = 0;   // pixels 0-127
  uint8_t cursor_y = 0;   // pixels 0-63

  // Status                  
  float calculate_signal_strength(rx_status &status);
  void update_display(rx_status & status, rx & receiver);
  bool frequency_autosave_pending = false;
  uint8_t frequency_autosave_timer = 10u;

  // Menu                    
  void print_option(const char options[], uint8_t option, uint8_t y_pos);
  uint32_t enumerate_entry(const char title[], const char options[], uint32_t max, uint32_t *value);
  uint32_t bit_entry(const char title[], const char options[], uint8_t bit_position, uint32_t *value);
  int16_t number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint32_t *value);
  bool frequency_entry();
  bool string_entry(char string[]);
  bool recall();
  bool store();
  bool upload();
  void autosave();
  void apply_settings(bool suspend);

  uint32_t regmode = 1;

  rx_settings &settings_to_apply;
  rx_status &status;
  rx &receiver;

  public:

  void autorestore();
  void do_ui(void);
  ui(rx_settings & settings_to_apply, rx_status & status, rx &receiver);

};

#endif
