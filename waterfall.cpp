#include "waterfall.h"

#include <cmath>
#include <cstdio>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "fft_filter.h"
#include "ili934x.h"
#include "font_seven_segment_25x15.h"
#include "font_16x12.h"
#include "font_8x5.h"
#include "rx_definitions.h"
#include "ui.h"


waterfall::waterfall()
{
    //using ili9341 library from here:
    //https://github.com/bizzehdee/pico-libs

    #define PIN_MISO 12
    #define PIN_CS   13
    #define PIN_SCK  14
    #define PIN_MOSI 15 
    #define PIN_DC   11
    #define PIN_RST  10
    #define SPI_PORT spi1
    spi_init(SPI_PORT, 75000000);
    //spi_init(SPI_PORT, 40000000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, PIN_RST, 320, 240, R0DEG);
}

waterfall::~waterfall()
{
    delete display;
}

void waterfall::configure_display(uint8_t settings, bool invert_colours)
{
    if(settings == 0)
    {
      enabled = false;
    } 
    else if(settings == 1)
    {
      enabled = true;
      display->setRotation(R0DEG, invert_colours);
    }
    else if(settings == 2)
    {
      enabled = true;
      display->setRotation(R180DEG, invert_colours);
    }
    else if(settings == 3)
    {
      enabled = true;
      display->setRotation(MIRRORED0DEG, invert_colours);
    }
    else if(settings == 4)
    {
      enabled = true;
      display->setRotation(MIRRORED180DEG, invert_colours);
    }
    else if(settings == 5)
    {
      enabled = true;
      display->setRotation(R90DEG, invert_colours);
    }
    else if(settings == 6)
    {
      enabled = true;
      display->setRotation(R270DEG, invert_colours);
    }
    else if(settings == 7)
    {
      enabled = true;
      display->setRotation(MIRRORED90DEG, invert_colours);
    }
    else if(settings == 8)
    {
      enabled = true;
      display->setRotation(MIRRORED270DEG, invert_colours);
    }

    display->init();
    if(enabled)
    {
       refresh = true;
       display->powerOn(true);
       draw();
    }
    else
    {
       display->powerOn(false);
    }
}

void waterfall::powerOn(bool state)
{
    power_state = state;
    if(enabled && state)
    {
       refresh = true;
       display->powerOn(true);
       draw();
    }
    else
    {
       display->powerOn(false);
    }
}

const uint16_t smeter_height = 165;
const uint16_t smeter_width = 56;
const uint16_t smeter_bar_width = 4;
const uint16_t smeter_x = 263;
const uint16_t smeter_y = 65;
const uint16_t waterfall_height = 80u;
const uint16_t waterfall_x = 1u;
const uint16_t waterfall_y = 157u;
const uint16_t num_cols = 256u;
const uint16_t scope_height = 80u;
const uint16_t scope_x = 1u;
const uint16_t scope_y = 61u;
const uint16_t dial_width = 320u;

void waterfall::draw()
{
    display->clear();

    //draw borders
    //Horizontal
    display->drawLine(0, waterfall_y-1, num_cols+1, waterfall_y-1, display->colour565(255,255,255));
    display->drawLine(0, waterfall_y+waterfall_height+1, num_cols+1, waterfall_y+waterfall_height+1, display->colour565(255,255,255));
    display->drawLine(0, scope_y+scope_height+1, num_cols+1, scope_y+scope_height+1, display->colour565(255,255,255));
    display->drawLine(0, scope_y-1,  num_cols+1, scope_y-1, display->colour565(255,255,255));

    //Vertical
    display->drawLine(0,  scope_y-1, 0,  scope_y+scope_height+1, display->colour565(255,255,255));
    display->drawLine(num_cols+1, scope_y-1, num_cols+1, scope_y+scope_height+1, display->colour565(255,255,255));
    display->drawLine(0,  waterfall_y-1, 0,  waterfall_y+waterfall_height+1, display->colour565(255,255,255));
    display->drawLine(num_cols+1, waterfall_y-1, num_cols+1, waterfall_y+waterfall_height+1, display->colour565(255,255,255));

    //smeter outline
    display->drawLine(smeter_x+28, smeter_y-2,               smeter_x+33, smeter_y-2,               display->colour565(255,255,255));
    display->drawLine(smeter_x+28, smeter_y+smeter_height+3, smeter_x+33, smeter_y+smeter_height+3, display->colour565(255,255,255));
    display->drawLine(smeter_x+28, smeter_y-1,               smeter_x+28, smeter_y+smeter_height+3, display->colour565(255,255,255));
    display->drawLine(smeter_x+33, smeter_y-1,               smeter_x+33, smeter_y+smeter_height+3, display->colour565(255,255,255));


    //draw smeter
    const char smeter[13][4]  = {"s0 ", "s1 ", "s2 ", "s3 ", "s4 ", "s5 ", "s6 ", "s7 ", "s8 ", "s9 ", "+10", "+20", "+30"};
    for(int16_t s=0; s<13; s++)
    {
      uint16_t s_px = dBm_to_px(S_to_dBm(s), smeter_height);
      uint16_t y = smeter_y+1 + smeter_height - s_px;
      display->drawLine(smeter_x+33, y, smeter_x+35,  y, display->colour565(255,255,255));
      uint16_t colour;
      switch(s)
      {
        case 12: colour = COLOUR_RED; break;
        case 11: colour = COLOUR_YELLOW; break;
        case 10: colour = COLOUR_GREEN; break;
        default: colour = COLOUR_WHITE; break;
      }
      display->drawString(smeter_x+smeter_width-18,  y-4, font_8x5, smeter[s], colour, COLOUR_BLACK);
    }
    for(int16_t dbm=-120; dbm<-40; dbm+=10)
    {
      uint16_t power_px = dBm_to_px(dbm, smeter_height);
      uint16_t y = smeter_y+1 + smeter_height - power_px;
      display->drawLine(smeter_x+26,  y, smeter_x+28,  y, display->colour565(255,255,255));
      char buffer[9];
      snprintf(buffer, 9, "%4i", dbm);
      display->drawString(smeter_x,  y-4, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);
    }

}

uint16_t waterfall::heatmap(uint8_t value, bool blend, bool highlight)
{
    uint8_t section = ((uint16_t)value*6)>>8;
    uint8_t fraction = ((uint16_t)value*6)&0xff;

    //blend colour e.g. for cursor
    uint8_t blend_r = 0;
    uint8_t blend_g = 255;
    uint8_t blend_b = 0;

    uint8_t r=0, g=0, b=0;
    switch(section)
    {
      case 0: //black->blue
        r=0;
        g=0;
        b=fraction;
        break;
      case 1: //blue->cyan
        r=0;
        g=fraction;
        b=255;
        break;
      case 2: //cyan->green
        r=0;
        g=255;
        b=255-fraction;
        break;
      case 3: //green->yellow
        r=fraction;
        g=255;
        b=0;
        break;
      case 4: //yellow->red
        r=255;
        g=255-fraction;
        b=0;
        break;
      case 5: //red->white
        r=255;
        g=fraction;
        b=fraction;
        break;
    }
    
    if(blend)
    {
      r = (uint16_t)r-(r>>2) + (blend_r>>2);
      g = (uint16_t)g-(g>>2) + (blend_g>>2);
      b = (uint16_t)b-(b>>2) + (blend_b>>2);
    }

    if(highlight)
    {
      r = (uint16_t)(r>>2) + blend_r - (blend_r>>2);
      g = (uint16_t)(g>>2) + blend_g - (blend_g>>2);
      b = (uint16_t)(b>>2) + blend_b - (blend_b>>2);
    }

    return display->colour565(r,g,b);
}

uint16_t waterfall::dBm_to_px(float power_dBm, int16_t px) {
  int16_t power = floorf((power_dBm-S0));
  power = power * px / (S9_10 + 20 - S0);
  if (power<0) power=0;
  if (power>px) power=px;
  return (power);
}

float waterfall::S_to_dBm(int S) {
  float dBm = 0;
  if (S<=9) {
    dBm = S0 + 6.0f * S;
  } else {
    dBm = S9_10 + (S-10) * 10.f;
  }
  return (dBm);
}

int waterfall::dBm_to_S(float power_dBm) {
  int power_s = floorf((power_dBm-S0)/6.0f);
  if(power_dBm >= S9) power_s = floorf((power_dBm-S9)/10.0f)+9;
  if(power_s < 0) power_s = 0;
  if(power_s > 12) power_s = 12;
  return (power_s);
}

void waterfall::update_spectrum(rx &receiver, rx_settings &settings, rx_status &status, uint8_t spectrum[], uint8_t dB10, uint8_t zoom)
{

    if(!enabled) return;
    if(!power_state) return;

    const uint16_t scope_fg = display->colour565(255, 255, 255);

    static uint16_t top_row = 0u;

    //scroll waterfall
    top_row = top_row?top_row-1:waterfall_height-1;

    //add new line to waterfall
    for(uint16_t col=0; col<num_cols; col++)
    {
      waterfall_buffer[top_row][col] = spectrum[col];
    }

    receiver.access(false);
    const int16_t power_dBm = status.signal_strength_dBm;
    receiver.release();

    static float filtered_power = power_dBm;
    filtered_power = (filtered_power * 0.7) + (power_dBm * 0.3);

    static float last_filtered_power = 0;
    static uint8_t last_squelch = 255;
    if(abs(filtered_power - last_filtered_power) > 1 || settings.squelch_threshold != last_squelch|| refresh)
    {
      last_filtered_power = filtered_power;
      last_squelch = settings.squelch_threshold;
      uint16_t power_px = dBm_to_px(filtered_power, smeter_height+6);
      uint16_t squelch_px = dBm_to_px(S_to_dBm(settings.squelch_threshold), smeter_height);

      uint16_t colour=heatmap(dBm_to_px(filtered_power, 255));
      display->fillRect(smeter_x+29, smeter_y+4+smeter_height-power_px, power_px, smeter_bar_width, colour);
      display->fillRect(smeter_x+29, smeter_y+1,    smeter_height-power_px, smeter_bar_width, COLOUR_BLACK);
      display->fillRect(smeter_x+29, smeter_y+0+smeter_height-squelch_px, 3, smeter_bar_width, COLOUR_RED);
      display->fillRect(smeter_x+29, smeter_y+1+smeter_height-squelch_px, 1, smeter_bar_width, COLOUR_WHITE);

      char buffer[9];
      snprintf(buffer, 9, "%4.0fdBm", filtered_power);
      display->drawString(233, 31, font_16x12, buffer, COLOUR_GREEN, COLOUR_BLACK);

    }

    //draw status
    static int16_t last_mode = -1;
    if(settings.mode != last_mode || refresh)
    {
      last_mode = settings.mode;
      const char modes[6][4]  = {"AM ", "AMS", "LSB", "USB", "FM ", "CW "};
      display->drawString(165, 31, font_16x12, modes[settings.mode], COLOUR_YELLOW, COLOUR_BLACK);
    }

    static uint8_t last_zoom = 255;
    if(zoom != last_zoom || refresh)
    {
      last_zoom = zoom;
      display->fillRect(waterfall_x,  waterfall_y-12, 8, 256, COLOUR_BLACK);

      const int32_t kHz_per_tick = zoom>=3?1:5;
      const int32_t bins_per_tick = (256*kHz_per_tick*zoom)/30;

      uint16_t freq_kHz = 0;
      for(uint16_t bin = 0; bin < 110; bin += bins_per_tick)
      {
        char buffer[7];
        sprintf(buffer, "%i", freq_kHz);
        uint16_t x = waterfall_x + num_cols/2 + bin - ((strlen(buffer)-1)*3);
        display->drawString(x,  waterfall_y-11, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);

        sprintf(buffer, "%i", -freq_kHz);
        x = waterfall_x + num_cols/2 - bin - ((strlen(buffer)-1)*6);
        display->drawString(x,  waterfall_y-11, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);
        freq_kHz += kHz_per_tick;
      }
    }

    //draw dial
    const uint8_t pixels_per_kHz = 5;
    const uint16_t scale_y = 0;

    static uint32_t last_frequency =0;
    if(settings.tuned_frequency_Hz != last_frequency || refresh)
    { 
      last_frequency = settings.tuned_frequency_Hz;

      display->fillRect(0, scale_y, 25, dial_width, COLOUR_BLACK);
      display->fillRect(0, scale_y+25, 1, dial_width, COLOUR_WHITE);
      display->fillRect(0, scale_y, 1, dial_width, COLOUR_WHITE);
      for(uint16_t x = 0; x<dial_width; x++)
      {
        uint32_t rounded_frequency_Hz = round(settings.tuned_frequency_Hz / 1000)*1000;
        uint32_t pixel_frequency_Hz = rounded_frequency_Hz + (((x-(dial_width/2))*1000)/pixels_per_kHz);
        //draw 1kHz tick Marks
        if(pixel_frequency_Hz%1000 == 0)
        {
          display->fillRect(x, scale_y+20, 5, 1, COLOUR_WHITE);
        }
        //draw 5kHz tick Marks
        if(pixel_frequency_Hz%5000 == 0)
        {
          display->fillRect(x, scale_y+17, 5, 1, COLOUR_WHITE);
        }
        //draw 10kHz tick Marks
        if(pixel_frequency_Hz%10000 == 0)
        {
          if(x > 20 && (x + 20) < dial_width)
          {
            char buffer[10];
            snprintf(buffer, 10, "%2lu.%02lu", pixel_frequency_Hz/1000000, ((pixel_frequency_Hz%1000000)/10000));
            display->drawString(x-18,  scale_y+5, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);
          }
          display->fillRect(x, scale_y+15, 10, 1, COLOUR_WHITE);
          display->fillRect(x, scale_y, 2, 1, COLOUR_WHITE);
        }
      }
      display->fillRect(dial_width/2-1, scale_y, 25, 3, COLOUR_RED);
      display->fillRect(dial_width/2, scale_y, 25, 1, COLOUR_WHITE);


    }

    //draw scope
    uint8_t data_points[num_cols];
    for(uint16_t scope_col=0; scope_col<num_cols; ++scope_col)
    {
      data_points[scope_col] = (scope_height * (uint16_t)waterfall_buffer[top_row][scope_col])/270;
    }
    uint16_t tick_spacing;
    if(zoom >= 3)
    {
      tick_spacing = 256*zoom/30; //place ticks at 1kHz steps
    }
    else
    {
      tick_spacing = 256*zoom*5/30; //place ticks at 5kHz steps
    }
    for(uint16_t scope_row = 0; scope_row < scope_height; ++scope_row)
    {
       uint16_t hline[num_cols];
       uint16_t waterfall_line[num_cols];
       uint16_t row_address = (top_row+scope_row)%waterfall_height;

       uint16_t scope_row_colour = heatmap((uint16_t)scope_row*256/scope_height, false, false);
       uint16_t scope_row_colour_passband = heatmap((uint16_t)scope_row*256/scope_height, true, false);
       uint16_t scope_row_colour_cursor = heatmap((uint16_t)scope_row*256/scope_height, true, true);
       const bool row_is_tick = (scope_row%(scope_height*dB10/270)) == 0;

       //draw one line of scope
       for(uint16_t scope_col=0; scope_col<num_cols; ++scope_col)
       {
 
         uint8_t data_point = data_points[scope_col];//(scope_height * (uint16_t)waterfall_buffer[top_row][scope_col])/270;

         const int16_t fbin = scope_col-128;
         const bool is_usb_col = (fbin > (status.filter_config.start_bin * zoom)) && (fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.upper_sideband;
         const bool is_lsb_col = (-fbin > (status.filter_config.start_bin * zoom)) && (-fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.lower_sideband;
         const bool is_passband = is_usb_col || is_lsb_col;
         const bool col_is_tick = (fbin%tick_spacing == 0) && fbin;
 
         if(scope_row < data_point)
         {
           uint16_t colour = scope_row_colour;
           colour = is_passband?scope_row_colour_passband:colour;
           colour = fbin==0?scope_row_colour_cursor:colour;
           hline[scope_col] = colour;
         }
         else if(scope_row == data_point)
         {
           hline[scope_col] = scope_fg;
         }
         else
         {
           uint16_t colour = heatmap(0, is_passband, fbin==0);
           colour = col_is_tick?COLOUR_GRAY:colour;
           colour = row_is_tick?COLOUR_GRAY:colour;
           hline[scope_col] = colour;
         }
       }
       display->dmaFlush();
       display->writeHLine(scope_x, scope_y+scope_height-1-scope_row, num_cols, hline);

       //draw 1 line of waterfall
       for(uint16_t scope_col=0; scope_col<num_cols; ++scope_col)
       {
         const int16_t fbin = scope_col-128;
         const bool is_usb_col = (fbin > (status.filter_config.start_bin * zoom)) && (fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.upper_sideband;
         const bool is_lsb_col = (-fbin > (status.filter_config.start_bin * zoom)) && (-fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.lower_sideband;
         const bool is_passband = is_usb_col || is_lsb_col;
 
         uint8_t heat = waterfall_buffer[row_address][scope_col];
         uint16_t colour=heatmap(heat, is_passband, fbin==0);
         waterfall_line[scope_col] = colour;
       }
       display->dmaFlush();
       display->writeHLine(waterfall_x, scope_row+waterfall_y, num_cols, waterfall_line);

     }
     display->dmaFlush();


    //draw frequency
    //extract frequency from status
    uint32_t remainder, MHz, kHz, Hz;
    MHz = (uint32_t)settings.tuned_frequency_Hz/1000000u;
    remainder = (uint32_t)settings.tuned_frequency_Hz%1000000u; 
    kHz = remainder/1000u;
    remainder = remainder%1000u; 
    Hz = remainder;
    //update frequency if changed
    static uint32_t lastMHz = 0;
    static uint32_t lastkHz = 0;
    static uint32_t lastHz = 0;
    if(lastMHz!=MHz || lastkHz!=kHz || lastHz!=Hz || refresh)
    {
      char buffer[20];
      snprintf(buffer, 20, "%02lu:%03lu:%03lu", MHz, kHz, Hz);
      display->drawString(0, 31, font_seven_segment_25x15, buffer, COLOUR_RED, COLOUR_BLACK);
      lastMHz = MHz;
      lastkHz = kHz;
      lastHz = Hz;
    }

    refresh = false;

}

