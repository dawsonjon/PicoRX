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
rx receiver(settings_to_apply, status);

void core1_main()
{
    receiver.run();
}

void apply_settings_get_status(bool settings_changed)
{
    multicore_fifo_push_blocking(settings_changed);
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
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
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
    const float signal_strength_dBFS = 20.0*log10((float)status.signal_amplitude / (8.0f * 2048.0f));//compared to adc_full_scale
    return full_scale_dBm - 60.0f + signal_strength_dBFS;
}

void update_display()
{
    char buff [16];
    ssd1306_clear(&disp);

    //frequency
    uint32_t remainder, MHz, kHz, Hz;
    MHz = (uint32_t)settings_to_apply.tuned_frequency_Hz/1000000u;
    remainder = (uint32_t)settings_to_apply.tuned_frequency_Hz%1000000u; 
    kHz = remainder/1000u;
    remainder = remainder%1000u; 
    Hz = remainder;
    snprintf(buff, 16, "%2u.%03u", MHz, kHz);
    ssd1306_draw_string(&disp, 0, 0, 2, buff);
    snprintf(buff, 16, ".%03u", Hz);
    ssd1306_draw_string(&disp, 72, 0, 1, buff);

    //signal strength
    const float power = calculate_signal_strength();

    //CPU 
    const float block_time = (float)adc_block_size/(float)adc_sample_rate;
    const float busy_time = block_time - ((float)status.idle_time*1e-6f);

    //mode
    const char am[]  = "AM";
    const char fm[]  = "FM";
    const char lsb[] = "LSB";
    const char usb[] = "USB";
    const char cw[] = "CW";
    char *modestr;
    switch(settings_to_apply.mode)
    {
      case AM: modestr = (char*)am; break;
      case LSB: modestr = (char*)lsb; break;
      case USB: modestr = (char*)usb; break;
      case FM: modestr = (char*)fm; break;
      case CW: modestr = (char*)cw; break;
    } 
    ssd1306_draw_string(&disp, 102, 0, 1, modestr);
    snprintf(buff, 16, "%2.0fdBm %2.0f%%", power, (100.0f*busy_time)/block_time);
    ssd1306_draw_string(&disp, 0, 16, 1, buff);

    //Display spectrum capture
    int16_t spectrum[128];
    int16_t offset;
    receiver.get_spectrum(spectrum, offset);
    ssd1306_draw_string(&disp, offset, 63-8, 1, "^");
    for(int x=0; x<128; x++)
    {
        int16_t y = (90-20.0*log10(spectrum[x]))/3;
        ssd1306_draw_pixel(&disp, x, y+32);
    }

    //update display
    ssd1306_show(&disp);
}


int main() {
    stdio_init_all();
    multicore_launch_core1(core1_main);    

    setup_display();
    setup_encoder();

    settings_to_apply.tuned_frequency_Hz = 1053e3;
    settings_to_apply.agc_speed = 3;
    settings_to_apply.mode = AM;
    settings_to_apply.step_Hz = 1000;

    while(1)
    {
      bool settings_changed = false;

      update_display();
      int encoder_change = get_encoder_change();
      if(encoder_change != 0)
      {
        settings_changed = true;
        settings_to_apply.tuned_frequency_Hz += encoder_change * settings_to_apply.step_Hz;
        printf("frequency: %08.0fHz\n", settings_to_apply.tuned_frequency_Hz);
      }
      
      char selection = getchar_timeout_us(0);
      float block_time, busy_time, power;

      switch(selection)
      {
        case 'f':
          int frequency; 
          puts("enter frequency>\n");
          scanf("%i", &frequency);
          printf("frequency: %iHz\n", frequency);
          settings_to_apply.tuned_frequency_Hz = frequency;
          settings_changed = true;
          break;

        case 'a':
          int speed; 
          puts("enter agc speed>\n");
          scanf("%i", &speed);
          printf("speed: %i\n", speed);
          settings_to_apply.agc_speed = speed;
          settings_changed = true;
          break;

        case 'm':
          int mode; 
          puts("enter mode>\n");
          scanf("%i", &mode);
          printf("mode: %i\n", mode);
          settings_to_apply.mode = mode;
          settings_changed = true;
          break;

        case 't':
          int step; 
          puts("enter step>\n");
          scanf("%i", &step);
          printf("step: %i\n", step);
          settings_to_apply.step_Hz = step;
          break;
        case 's':
          power = 20.0*log10((float)status.signal_amplitude / (4.0f * 2048.0f));//compared to adc_full_scale
          printf("signal strength: %2.0fdBFS\n", power);
          break;

        case 'b':
          reset_usb_boot(0,0);
          break;

        case 'c':
          block_time = (float)adc_block_size/(float)adc_sample_rate;
          busy_time = block_time - ((float)status.idle_time*1e-6f);
          printf("cpu usage: %2.0f%%\n", (100.0f * busy_time)/block_time);
          break;

        default:
          break;
      }

      sleep_us(100000);
      apply_settings_get_status(settings_changed);

    }

}
