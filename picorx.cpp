#include "pico/stdlib.h"
#include <stdio.h>

#include "pico/multicore.h"
#include "pico/time.h"

#include "rx.h"
#include "ui.h"
#include "waterfall.h"
#include "cat.h"

#define UI_REFRESH_HZ (10UL)
#define UI_REFRESH_US (1000000UL / UI_REFRESH_HZ)
#define CAT_REFRESH_US (1000UL) //10ms
#define BUTTONS_REFRESH_US (50000UL) // 50ms <=> 20Hz
#define WATERFALL_REFRESH_US (50000UL) // 50ms <=> 20Hz

uint8_t spectrum[256];
uint8_t dB10=10;
uint8_t zoom=1;
static rx_settings settings_to_apply;
static rx_status status;

static rx receiver(settings_to_apply, status);
waterfall waterfall_inst;
static ui user_interface(settings_to_apply, status, receiver, spectrum, dB10, zoom, waterfall_inst);

void core1_main()
{
    multicore_lockout_victim_init();
    receiver.run();
}

int main() 
{
  stdio_init_all();
  multicore_launch_core1(core1_main);
  stdio_init_all();

  gpio_set_function(10, GPIO_FUNC_SIO);
  gpio_set_dir(10, GPIO_OUT);
  gpio_put(10, 1);

  // create an alarm pool for USB streaming with highest priority (0), so
  // that it can pre-empt the default pool
  receiver.set_alarm_pool(alarm_pool_create(0, 16));
  user_interface.autorestore();


  uint32_t last_ui_update = 0;
  uint32_t last_cat_update = 0;
  uint32_t last_buttons_update = 0;
  uint32_t last_waterfall_update = 0;

  while(1)
  {
    user_interface.update_paddles();
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
      receiver.get_spectrum(spectrum, dB10, zoom);
    }

    if(time_us_32() - last_cat_update > CAT_REFRESH_US)
    {
      last_cat_update = time_us_32();
      process_cat_control(settings_to_apply, status, receiver, user_interface.get_settings());
    }

    if(time_us_32() - last_waterfall_update > WATERFALL_REFRESH_US)
    {
      last_waterfall_update = time_us_32();
      waterfall_inst.update_spectrum(receiver, user_interface.get_settings(), settings_to_apply, status, spectrum, dB10, zoom);
    }


  }
}
