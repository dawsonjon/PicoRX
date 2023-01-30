#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"

#include "quadrature_encoder.pio.h"
#include "ssd1306.h"
#include "font.h"
#include "rx.h"
#include "ui.h"

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
    const float busy_time = ((float)status.busy_time*1e-6f);

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
    setup_buttons();

    //Apply default settings
    settings[idx_frequency] = 198e3;
    settings[idx_agc_speed] = 3;
    settings[idx_mode] = AM;
    settings[idx_step] = 3;
    settings[idx_cw_sidetone] = 1000;

    bool settings_changed = true;
    while(1)
    {

      settings_changed = do_ui() || settings_changed;
      if(settings_changed)
      {
        settings_to_apply.tuned_frequency_Hz = settings[idx_frequency];
        settings_to_apply.agc_speed = settings[idx_agc_speed];
        settings_to_apply.mode = settings[idx_mode];
        settings_to_apply.step_Hz = step_sizes[settings[idx_step]];
        settings_to_apply.cw_sidetone_Hz = settings[idx_cw_sidetone];
      }

      update_display();
      apply_settings_get_status(settings_changed);
      settings_changed = false;
      sleep_us(100000);

    }

}
