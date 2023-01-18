#include "pico/stdlib.h"
#include "pico/multicore.h"

// Our assembled program:
#include "rx.h"

rx_settings settings_to_apply;

void core1_main()
{
    rx receiver(settings_to_apply);
    settings_to_apply.tuned_frequency_Hz = 1215e3;
    receiver.run();
}

int main() {
    stdio_init_all();
    multicore_launch_core1(core1_main);    

    while(1)
    {
      puts("menu\n");
      puts("f: frequency\n");
      puts("a: agc_speed\n");
      char selection; 
      scanf("%c", &selection);

      switch(selection)
      {
        case 'f':
          int frequency; 
          puts("enter frequency>\n");
          scanf("%i", &frequency);
          printf("frequency: %iHz", frequency);
          settings_to_apply.tuned_frequency_Hz = frequency;
          break;

        case 'a':
          int speed; 
          puts("enter agc speed>\n");
          scanf("%i", &speed);
          printf("speed: %i", speed);
          settings_to_apply.agc_speed = speed;
          break;

        default:
          continue;
      }

      multicore_fifo_push_blocking(0);
      multicore_fifo_pop_blocking();
    }

}
