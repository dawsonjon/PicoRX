#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "rx.h"
#include "ui.h"

rx_settings settings_to_apply;
rx_status status;
rx receiver(settings_to_apply, status);
ui user_interface(settings_to_apply, status,  receiver);

void core1_main()
{
    receiver.run();
}


int main() {
  stdio_init_all();
  multicore_launch_core1(core1_main);    

  bool settings_changed = true;
  while(1)
  {
    user_interface.do_ui(settings_changed);
    settings_changed = false;
    sleep_us(100000);
  }
}
