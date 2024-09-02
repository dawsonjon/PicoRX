#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/multicore.h"
#include "rx.h"
#include "ui.h"

#define UI_REFRESH_HZ (10UL)
#define UI_REFRESH_US (1000000UL / UI_REFRESH_HZ)

static rx_settings settings_to_apply;
static rx_status status;
static rx receiver(settings_to_apply, status);
static ui user_interface(settings_to_apply, status,  receiver);

void core1_main()
{
    multicore_lockout_victim_init();
    receiver.run();
}

int main() 
{
  stdio_init_all();
  multicore_launch_core1(core1_main);    

  //sleep_us(5000000);
  user_interface.autorestore();

  while(1)
  {
    uint32_t _time_us = time_us_32();
    user_interface.do_ui();
    _time_us = time_us_32() - _time_us;
    if (_time_us < UI_REFRESH_US)
      sleep_us(UI_REFRESH_US - _time_us);
  }
}
