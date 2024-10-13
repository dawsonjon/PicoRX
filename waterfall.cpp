#include "waterfall.h"

#include <cmath>
#include <cstdio>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "fft_filter.h"
#include "ili934x.h"
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
    //spi_init(SPI_PORT, 62500000);
    spi_init(SPI_PORT, 40000000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, PIN_RST, 240, 320, MIRRORED90DEG);
}

waterfall::~waterfall()
{
    delete display;
}

void waterfall::configure_display(uint8_t settings)
{
    if(settings == 0)
    {
      enabled = false;
    } 
    else if(settings == 1)
    {
      enabled = true;
      display->setRotation(MIRRORED90DEG);
    }
    else if(settings == 2)
    {
      enabled = true;
      display->setRotation(MIRRORED270DEG);
    }
    else if(settings == 3)
    {
      enabled = true;
      display->setRotation(R90DEG);
    }
    else if(settings == 4)
    {
      enabled = true;
      display->setRotation(R270DEG);
    }

    if(enabled) draw();
}

void waterfall::draw()
{
    display->init();
    display->clear();

    //draw borders
    //Horizontal
    display->drawLine(31, 135, 288, 135, display->colour565(255,255,255));
    display->drawLine(31, 122, 288, 122, display->colour565(255,255,255));
    display->drawLine(31, 20,   288, 20, display->colour565(255,255,255));
    display->drawLine(31, 239, 288, 239, display->colour565(255,255,255));
    display->drawLine(292,  20, 319,  20, display->colour565(255,255,255));
    display->drawLine(292, 239, 319, 239, display->colour565(255,255,255));
    display->drawLine(0,  20, 27,  20, display->colour565(255,255,255));
    display->drawLine(0, 239, 27, 239, display->colour565(255,255,255));

    //Vertical
    display->drawLine(31,  20, 31,  122, display->colour565(255,255,255));
    display->drawLine(288, 20, 288, 122, display->colour565(255,255,255));
    display->drawLine(31,  135, 31,  239, display->colour565(255,255,255));
    display->drawLine(288, 135, 288, 239, display->colour565(255,255,255));
    display->drawLine(292, 20, 292, 239, display->colour565(255,255,255));
    display->drawLine(319, 20, 319, 239, display->colour565(255,255,255));
    display->drawLine(0,   20, 0,   239, display->colour565(255,255,255));
    display->drawLine(27,  20, 27,  239, display->colour565(255,255,255));

    //5kHz ticks
    for(uint16_t fbin=0; fbin<256; ++fbin)
    {
      if((fbin-128)%42==0)
      {
        display->drawLine(32+fbin, 122, 32+fbin, 123, COLOUR_WHITE);
      }
    }
    display->drawString(29,  127, font_8x5, "-15", COLOUR_WHITE, COLOUR_BLACK);
    display->drawString(70,  127, font_8x5, "-10", COLOUR_WHITE, COLOUR_BLACK);
    display->drawString(111, 127, font_8x5, "-5",  COLOUR_WHITE, COLOUR_BLACK);
    display->drawString(154, 127, font_8x5, "-0",  COLOUR_WHITE, COLOUR_BLACK);
    display->drawString(199, 127, font_8x5, "5",   COLOUR_WHITE, COLOUR_BLACK);
    display->drawString(238, 127, font_8x5, "10",  COLOUR_WHITE, COLOUR_BLACK);
    display->drawString(279, 127, font_8x5, "15",  COLOUR_WHITE, COLOUR_BLACK);

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
      r = (uint16_t)r-(r>>1) + (blend_r>>1);
      g = (uint16_t)g-(g>>1) + (blend_g>>1);
      b = (uint16_t)b-(b>>1) + (blend_b>>1);
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

void waterfall::update_spectrum(rx &receiver, rx_settings &settings, rx_status &status, uint8_t spectrum[], uint8_t dB10)
{

    if(!enabled) return;

    const uint16_t waterfall_height = 100u;
    const uint16_t waterfall_x = 32u;
    const uint16_t waterfall_y = 136u;
    const uint16_t num_cols = 256u;
    const uint16_t scope_height = 100u;
    const uint16_t scope_x = 32u;
    const uint16_t scope_y = 21u;
    const uint16_t scope_fg = display->colour565(255, 255, 255);

    enum FSM_states{
      update_waterfall,
      draw_smeter,
      draw_status,
      draw_waterfall, 
      draw_scope, 
      draw_frequency
    };
    static uint16_t top_row = 0u;
    static uint8_t waterfall_row = 0u;
    static uint8_t scope_col = 0u;
    static uint8_t stored_dB10 = 0u;
    static FSM_states FSM_state = update_waterfall;

    if(FSM_state == update_waterfall)
    {
      //scroll waterfall
      top_row = top_row?top_row-1:waterfall_height-1;

      //add new line to waterfall
      for(uint16_t col=0; col<num_cols; col++)
      {
        waterfall_buffer[top_row][col] = spectrum[col];
      }

      FSM_state = draw_smeter;

    } else if(FSM_state == draw_smeter ){

      const uint16_t smeter_height = 237-43;
      const uint16_t smeter_width = 24;

      receiver.access(false);
      const int16_t power_dBm = status.signal_strength_dBm;
      receiver.release();

      static float filtered_power = power_dBm;
      filtered_power = (filtered_power * 0.7) + (power_dBm * 0.3);

      static float last_filtered_power = 0;
      if(abs(filtered_power - last_filtered_power) > 1)
      {
        last_filtered_power = filtered_power;
        uint16_t power_px = dBm_to_px(filtered_power, smeter_height);
        uint16_t squelch_px = dBm_to_px(S_to_dBm(settings.squelch), smeter_height);

        uint16_t colour=heatmap(dBm_to_px(filtered_power, 255));
        display->fillRect(294, 43+smeter_height-power_px, power_px, smeter_width, colour);
        display->fillRect(294, 43, smeter_height-power_px, smeter_width, COLOUR_BLACK);
        display->drawLine(294, 43+smeter_height-squelch_px, 316, 43+smeter_height-squelch_px, COLOUR_WHITE);

        char buffer[9];
        snprintf(buffer, 9, "%4.0fdBm", filtered_power);
        display->drawString(236, 0, font_16x12, buffer, COLOUR_FUCHSIA, COLOUR_BLACK);

        uint16_t power_s = dBm_to_S(filtered_power);
        if(power_s >= 0 && power_s <= 9)
        {
          snprintf(buffer, 9, "S%1u", power_s);
          display->drawString(293, 22, font_16x12, buffer, COLOUR_WHITE, COLOUR_BLACK);
          display->drawString(296, 38, font_8x5, "   ", COLOUR_WHITE, COLOUR_BLACK);
        }
        else
        {
          display->drawString(293, 22, font_16x12, "S9", COLOUR_WHITE, COLOUR_BLACK);
          snprintf(buffer, 9, "+%1u", (power_s-9)*10);
          display->drawString(296, 38, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);
        }
      }

      FSM_state = draw_status;
    } else if(FSM_state == draw_status ){

      static int16_t last_mode = -1;
      if(settings.mode != last_mode)
      {
        last_mode = settings.mode;
        const char modes[6][4]  = {"AM ", "AMS", "LSB", "USB", "FM ", "CW "};
        display->drawString(0, 0, font_16x12, modes[settings.mode], COLOUR_FUCHSIA, COLOUR_BLACK);
      }

      FSM_state = draw_waterfall;
    } else if(FSM_state == draw_waterfall ){
    
      //draw one line of waterfall
      uint16_t line[num_cols];
      uint16_t row_address = (top_row+waterfall_row)%waterfall_height;
      for(uint16_t col=0; col<num_cols; ++col)
      {
         const int16_t fbin = col-128;
         const bool is_usb_col = (fbin > status.filter_config.start_bin) && (fbin < status.filter_config.stop_bin) && status.filter_config.upper_sideband;
         const bool is_lsb_col = (-fbin > status.filter_config.start_bin) && (-fbin < status.filter_config.stop_bin) && status.filter_config.lower_sideband;
         const bool is_passband = is_usb_col || is_lsb_col;

         uint8_t heat = waterfall_buffer[row_address][col];
         uint16_t colour=heatmap(heat, is_passband, fbin==0);
         line[col] = colour;
      }
      display->writeHLine(waterfall_x, waterfall_row+waterfall_y, num_cols, line);

      if(waterfall_row == waterfall_height-1)
      {
        FSM_state = draw_scope;
        stored_dB10 = dB10;
        waterfall_row = 0;
      } else {
        ++waterfall_row;
      }

    } else if(FSM_state == draw_scope) {

      //draw scope one bar at a time
      uint8_t data_point = (scope_height * (uint16_t)waterfall_buffer[top_row][scope_col])/270;
      uint16_t vline[scope_height];
  
      const int16_t fbin = scope_col-128;
      const bool is_usb_col = (fbin > status.filter_config.start_bin) && (fbin < status.filter_config.stop_bin) && status.filter_config.upper_sideband;
      const bool is_lsb_col = (-fbin > status.filter_config.start_bin) && (-fbin < status.filter_config.stop_bin) && status.filter_config.lower_sideband;
      const bool is_passband = is_usb_col || is_lsb_col;
      const bool col_is_tick = (fbin%42 == 0) && fbin;


      for(uint8_t row=0; row<scope_height; ++row)
      {
        const bool row_is_tick = (row%(4*scope_height*stored_dB10/270)) == 0;
        if(row < data_point)
        {
          vline[scope_height - 1 - row] = heatmap((uint16_t)row*256/scope_height, is_passband, fbin==0);
        }
        else if(row == data_point)
        {
          vline[scope_height - 1 - row] = scope_fg;
        }
        else
        {
          uint16_t colour = heatmap(0, is_passband, fbin==0);
          colour = col_is_tick?COLOUR_GRAY:colour;
          colour = row_is_tick?COLOUR_GRAY:colour;
          vline[scope_height - 1 - row] = colour;
        }
      }
      display->writeVLine(scope_x+scope_col, scope_y, scope_height, vline);

      if(scope_col == num_cols-1)
      {
        //scroll waterfall
        FSM_state = draw_frequency;
        scope_col = 0;
      } else {
        ++scope_col;
      }

    } else if(FSM_state == draw_frequency) {

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
      if(lastMHz!=MHz || lastkHz!=kHz || lastHz!=Hz)
      {
        char buffer[20];
        snprintf(buffer, 20, "%2lu.%03lu.%03lu", MHz, kHz, Hz);
        display->drawString(100, 0, font_16x12, buffer, COLOUR_WHITE, COLOUR_BLACK);
        lastMHz = MHz;
        lastkHz = kHz;
        lastHz = Hz;
      }
      FSM_state = update_waterfall;
    }
}

