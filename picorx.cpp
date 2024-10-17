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
#define CAT_REFRESH_US (10000UL)

uint8_t spectrum[256];
uint8_t dB10=10;
static rx_settings settings_to_apply;
static rx_status status;
static rx receiver(settings_to_apply, status);
waterfall waterfall_inst;
static ui user_interface(settings_to_apply, status, receiver, spectrum, dB10, waterfall_inst);

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

  // create an alarm pool for USB streaming with highest priority (0), so
  // that it can pre-empt the default pool
  receiver.set_alarm_pool(alarm_pool_create(0, 16));


  sleep_ms(10000);

  user_interface.autorestore();

  uint32_t last_ui_update = 0;
  uint32_t last_cat_update = 0;
  while(1)
  {
    //schedule tasks
    if(time_us_32() - last_ui_update > UI_REFRESH_US)
    {
      last_ui_update = time_us_32();
      user_interface.do_ui();
      receiver.get_spectrum(spectrum, dB10);
    }

    else if(time_us_32() - last_cat_update > CAT_REFRESH_US)
    {
      process_cat_control(settings_to_apply, status, receiver, user_interface.get_settings());
    }

    waterfall_inst.update_spectrum(receiver, settings_to_apply, status, spectrum, dB10);

  }
}
