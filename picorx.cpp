#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/multicore.h"
#include "rx.h"
#include "ui.h"

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
    user_interface.do_ui();
    sleep_us(100000);
  }
}
