#ifndef __ui_h__
#define __ui_h__

#include <cstdint>

#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "quadrature_encoder.pio.h"
#include "ssd1306.h"
#include "font.h"
#include "rx.h"

const uint8_t PIN_AB = 20;
const uint8_t PIN_B  = 21;
const uint8_t PIN_MENU = 22;
const uint8_t PIN_BACK = 17;
const uint8_t PIN_DISPLAY_SDA = 18;
const uint8_t PIN_DISPLAY_SCL = 19;

// settings that get stored in eeprom
#define idx_frequency 0
#define idx_mode 1
#define idx_agc_speed 2
#define idx_step 3
#define idx_squelch 4
#define idx_volume 5
#define idx_max_frequency 6
#define idx_min_frequency 7
#define idx_cw_sidetone 8
#define idx_cw_speed 9
#define idx_pps_count 10

// settings that are transient
#define idx_band 11
#define idx_test_signal 12
#define idx_USB_audio 13
#define idx_tx 14
#define idx_mute 15

// define wait macros
#define WAIT_10MS sleep_us(10000);
#define WAIT_100MS sleep_us(100000);
#define WAIT_500MS sleep_us(500000);;

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

  // Display
  void setup_display();
  void display_clear();
  void display_line1();
  void display_line2();
  void display_write(char x);
  void display_print(const char str[]);
  void display_print_num(const char format[], int16_t num);
  void display_show();

  ssd1306_t disp;
  uint8_t cursor_x = 0;
  uint8_t cursor_y = 0;

  // Status                  
  float calculate_signal_strength(rx_status &status);
  void update_display(rx_status & status, rx & receiver);

  // Menu                    
  void print_option(const char options[], uint8_t option);
  uint32_t enumerate_entry(const char title[], const char options[], uint32_t max, uint32_t *value);
  int16_t number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint32_t *value);
  bool frequency_entry();

  rx_settings &settings_to_apply;
  rx_status &status;
  rx &receiver;

  public:

  bool do_ui(bool rx_settings_changed);
  ui(rx_settings & settings_to_apply, rx_status & status, rx &receiver);

};

#endif
