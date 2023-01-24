#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"

#include "quadrature_encoder.pio.h"
#include "ssd1306.h"
#include "font.h"
#include "rx.h"

rx_settings settings_to_apply;
rx_status status;

void core1_main()
{
    rx receiver(settings_to_apply, status);
    receiver.run();
}

void apply_settings_get_status()
{
    multicore_fifo_push_blocking(0);
    multicore_fifo_pop_blocking();
}

ssd1306_t disp;
void setup_display() {
    i2c_init(i2c1, 400000);
    gpio_set_function(18, GPIO_FUNC_I2C);
    gpio_set_function(19, GPIO_FUNC_I2C);
    gpio_pull_up(18);
    gpio_pull_up(19);
    disp.external_vcc=false;
    ssd1306_init(&disp, 128, 32, 0x3C, i2c1);
}

int new_position;
int old_position;
const uint PIN_AB = 20;
const uint sm = 0;
PIO pio = pio1;
void setup_encoder()
{
    uint offset = pio_add_program(pio, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio, sm, offset, PIN_AB, 1000);
    new_position = (quadrature_encoder_get_count(pio, sm) + 2)/4;
    old_position = new_position;
}

int get_encoder_change()
{
    new_position = (quadrature_encoder_get_count(pio, sm) + 2)/4;
    int delta = new_position - old_position;
    old_position = new_position;
    return delta;
}

float calculate_signal_strength()
{
    const float full_scale_rms_mW = (0.5f * 0.707f * 1000.0f * 3.3f * 3.3f) / 50.0f;
    const float full_scale_dBm = 10.0f * log10(full_scale_rms_mW);
    const float signal_strength_dBFS = 20.0*log10((float)status.signal_amplitude / (4.0f * 2048.0f));//compared to adc_full_scale
    return full_scale_dBm - 60.0f + signal_strength_dBFS;
}

void update_display()
{
    char buff [11];
    ssd1306_clear(&disp);

    //frequency
    uint32_t remainder, MHz, kHz, Hz;
    MHz = (uint32_t)settings_to_apply.tuned_frequency_Hz/1000000u;
    remainder = (uint32_t)settings_to_apply.tuned_frequency_Hz%1000000u; 
    kHz = remainder/1000u;
    remainder = remainder%1000u; 
    Hz = remainder;
    snprintf(buff, 11, "%02u.%03u.%03u", MHz, kHz, Hz);
    ssd1306_draw_string(&disp, 0, 0, 1, buff);

    //signal strength
    const float power = calculate_signal_strength();
    snprintf(buff, 11, "%2.0fdBm\n", power);
    ssd1306_draw_string(&disp, 0, 16, 1, buff);

    //update display
    ssd1306_show(&disp);
}

int main() {
    stdio_init_all();
    multicore_launch_core1(core1_main);    

    setup_display();
    setup_encoder();

    settings_to_apply.tuned_frequency_Hz = 1050e3;
    settings_to_apply.agc_speed = 3;

    while(1)
    {

      update_display();

      int encoder_change = get_encoder_change();
      if(encoder_change != 0)
      {
        settings_to_apply.tuned_frequency_Hz += encoder_change * 100;
        printf("frequency: %08.0fHz\n", settings_to_apply.tuned_frequency_Hz);
        apply_settings_get_status();
      }

      char selection = getchar_timeout_us(100);
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

        case 'b':
          reset_usb_boot(0,0);
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
