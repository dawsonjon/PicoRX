#include "pico/stdlib.h"
#include <stdio.h>

#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/exception.h"
#include "hardware/watchdog.h"

#include "rx.h"
#include "ui.h"
#include "waterfall.h"
#include "cat.h"
#include "stack_watermark.h"
#include "sdcard.h"

#define UI_REFRESH_HZ (10UL)
#define UI_REFRESH_US (1000000UL / UI_REFRESH_HZ)
#define CAT_REFRESH_US (1000UL) //1ms
#define BUTTONS_REFRESH_US (50000UL) // 50ms <=> 20Hz
#define WATERFALL_REFRESH_US (50000UL) // 50ms <=> 20Hz
#define STACK_UPDATE_US (1000000UL) // 1s

uint8_t spectrum[256];
uint8_t audio[128];
uint8_t dB10=10;
uint8_t zoom=1;
static rx_settings settings_to_apply;
static rx_status status;
static rx receiver(settings_to_apply, status);
static waterfall waterfall_inst(receiver);
static ui user_interface(settings_to_apply, status, receiver, spectrum, audio, dB10, zoom, waterfall_inst);

void core1_main()
{
    multicore_lockout_victim_init();
    receiver.run();
}

int main()
{

  // emergency bootloader mode
  gpio_init(PIN_BACK);
  gpio_set_dir(PIN_BACK, GPIO_IN);
  gpio_pull_up(PIN_BACK);
  if (gpio_get(PIN_BACK) == false) {
    reset_usb_boot(0, 0);
  }

  stdio_init_all();
  init_stack_watermark();
  watchdog_enable(2000, true);
  multicore_launch_core1(core1_main);

  // create an alarm pool for USB streaming with highest priority (0), so
  // that it can pre-empt the default pool
  receiver.set_alarm_pool(alarm_pool_create(0, 16));
  user_interface.autorestore();

  bool sd_card_mounted = sdcard_init(user_interface.get_settings().global.sd_card_counter);
  user_interface.set_sd_card_icon(sd_card_mounted);

  uint32_t last_ui_update = 0;
  uint32_t last_cat_update = 0;
  uint32_t last_buttons_update = 0;
  uint32_t last_waterfall_update = 0;
  uint32_t last_stack_update = 0;

  s_settings s = user_interface.get_settings();
  bool sd_card_save = s.global.sd_card_save;
  if (sd_card_save) {
    const uint32_t c = sdcard_start_recording(s.channel.frequency, s.channel.mode);
    user_interface.update_sdcard_counter(c);
  }

  while(1)
  {

    watchdog_update();

    //schedule tasks
    if (time_us_32() - last_buttons_update > BUTTONS_REFRESH_US)
    {
      last_buttons_update = time_us_32();
      user_interface.update_buttons();
    }
    receiver.tune();

    if(time_us_32() - last_ui_update > UI_REFRESH_US)
    {
      last_ui_update = time_us_32();
      user_interface.do_ui();
      s = user_interface.get_settings();
      if (sd_card_save != s.global.sd_card_save) {
        sd_card_save = s.global.sd_card_save;
        if (sd_card_save) {
          const uint32_t c = sdcard_start_recording(s.channel.frequency, s.channel.mode);
          user_interface.update_sdcard_counter(c);
        } else {
          sdcard_stop_recording();
        }
      }
      receiver.get_spectrum(spectrum, dB10, zoom);
      receiver.get_audio(audio);
    }

    if(time_us_32() - last_cat_update > CAT_REFRESH_US)
    {
      last_cat_update = time_us_32();
      process_cat_control(settings_to_apply, status, receiver, user_interface.get_settings());
    }

    if(time_us_32() - last_waterfall_update > WATERFALL_REFRESH_US)
    {
      last_waterfall_update = time_us_32();
      waterfall_inst.update(user_interface.get_settings(), settings_to_apply, status, spectrum, dB10, zoom);
    }

    if(time_us_32() - last_stack_update > STACK_UPDATE_US)
    {
      last_stack_update = time_us_32();
      #ifdef PRINT_STACK_USAGE
      print_stack_usage();
      #endif
    }

    if (sd_card_save) {
      if (sdcard_needs_flush()) {
        sdcard_flush();
      }
    }
  }
}
