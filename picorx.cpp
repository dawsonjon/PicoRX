#include "pico/stdlib.h"
#include "pico/multicore.h"

// Our assembled program:
#include "rx.h"

rx_settings settings_to_apply;
rx_status status;

void core1_main()
{
    rx receiver(settings_to_apply, status);
    settings_to_apply.tuned_frequency_Hz = 1050e3;
    receiver.run();
}

void apply_settings_get_status()
{
    multicore_fifo_push_blocking(0);
    multicore_fifo_pop_blocking();
}

int main() {
    stdio_init_all();
    multicore_launch_core1(core1_main);    

    while(1)
    {

      char selection; 
      scanf("%c", &selection);

      float block_time, busy_time, power;

      switch(selection)
      {
        case 'f':
          int frequency; 
          puts("enter frequency>\n");
          scanf("%i", &frequency);
          printf("frequency: %iHz\n", frequency);
          settings_to_apply.tuned_frequency_Hz = frequency;
          apply_settings_get_status();
          break;

        case 'a':
          int speed; 
          puts("enter agc speed>\n");
          scanf("%i", &speed);
          printf("speed: %i\n", speed);
          settings_to_apply.agc_speed = speed;
          apply_settings_get_status();
          break;

        case 's':
          apply_settings_get_status();
          power = 20.0*log10((float)status.signal_amplitude / (4.0f * 2048.0f));//compared to adc_full_scale
          printf("signal strength: %2.0fdBFS\n", power);
          break;

        case 'c':
          apply_settings_get_status();
          block_time = (float)adc_block_size/(float)adc_sample_rate;
          busy_time = block_time - ((float)status.idle_time*1e-6f);
          printf("cpu usage: %2.0f%%\n", (100.0f * busy_time)/block_time);
          break;

        default:
          continue;
      }

    }

}
