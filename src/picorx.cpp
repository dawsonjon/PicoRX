#include "pico/stdlib.h"
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/exception.h"
#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "aux_display.h"
#include "cat.h"
#include "stack_watermark.h"
#include "ui.h"
#include "xcvr.h"

#define UI_REFRESH_HZ (10UL)
#define UI_REFRESH_US (1000000UL / UI_REFRESH_HZ)
#define CAT_REFRESH_US (1000UL)        // 1ms
#define BUTTONS_REFRESH_US (50000UL)   // 50ms <=> 20Hz
#define WATERFALL_REFRESH_US (50000UL) // 50ms <=> 20Hz
#define STACK_UPDATE_US (1000000UL)    // 1s

uint8_t spectrum[256];
uint8_t hold[256];
uint8_t audio[128];
uint8_t dB10 = 10;
uint8_t zoom = 1;
static xcvr_settings settings_to_apply;
static xcvr_status status;
static xcvr transceiver(settings_to_apply, status);
static s_settings ui_settings = default_settings;
static c_aux_display aux_display(transceiver, ui_settings, settings_to_apply, status);
static ui user_interface(ui_settings, settings_to_apply, status, transceiver, spectrum, hold, audio,
                         dB10, zoom, aux_display);

void core1_main()
{
  multicore_lockout_victim_init();
  transceiver.run();
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
  watchdog_enable(10000, true);
  multicore_launch_core1(core1_main);

  // run SPI from USB clock so that it isn't affected by sys_clk changes
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB, 48 * MHZ,
                  48 * MHZ);

  // create an alarm pool for USB streaming with highest priority (0), so
  // that it can pre-empt the default pool
  transceiver.set_alarm_pool(alarm_pool_create(0, 16));
  user_interface.autorestore();

  uint32_t last_ui_update = 0;
  uint32_t last_cat_update = 0;
  uint32_t last_waterfall_update = 0;
  uint32_t last_stack_update = 0;

  while (1) {

    watchdog_update();

    // schedule tasks
    if (sem_try_acquire(&transceiver.i2c_semaphore)) {
      if (time_us_32() - last_ui_update > UI_REFRESH_US) {
        last_ui_update = time_us_32();
        user_interface.do_ui();
        transceiver.get_spectrum(spectrum, hold, dB10, zoom);
        transceiver.get_audio(audio);
      }
      sem_release(&transceiver.i2c_semaphore);
    }

    if (time_us_32() - last_cat_update > CAT_REFRESH_US) {
      last_cat_update = time_us_32();
      process_cat_control(settings_to_apply, status, transceiver, user_interface.get_settings());
    }

    // if(sem_try_acquire(&transceiver.i2c_semaphore))  {
    if (time_us_32() - last_waterfall_update > WATERFALL_REFRESH_US) {
      last_waterfall_update = time_us_32();
      aux_display.update(spectrum, hold, dB10, zoom);
    }
    sem_release(&transceiver.i2c_semaphore);
    //}

    if (time_us_32() - last_stack_update > STACK_UPDATE_US) {
      last_stack_update = time_us_32();
#ifdef PRINT_STACK_USAGE
      print_stack_usage();
#endif
    }
  }
}
