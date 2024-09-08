#include "waterfall.h"

#include <cmath>
#include <cstdio>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ili934x.h"
#include "FreeMono12pt7b.h"

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
    display->reset();
    display->init();
    display->clear();
    //display->drawLine(31, 135, 288, 135, display->colour565(255,255,255));
    //display->drawLine(31, 0, 288, 0, display->colour565(255,255,255));
    //display->drawLine(31, 239, 288, 239, display->colour565(255,255,255));
    //display->drawLine(31, 0, 31, 239, display->colour565(255,255,255));
    //display->drawLine(288, 0, 288, 239, display->colour565(255,255,255));
}

waterfall::~waterfall()
{
    delete display;
}

uint16_t waterfall::heatmap(uint8_t value, bool blend, bool highlight)
{
    uint8_t section = ((uint16_t)value*6)>>8;
    uint8_t fraction = ((uint16_t)value*6)&0xff;

    //blend colour e.g. for cursor
    uint8_t blend_r = 14;
    uint8_t blend_g = 158;
    uint8_t blend_b = 53;

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

void waterfall::new_spectrum(uint8_t spectrum[], s_filter_control &fc, uint16_t MHz, uint16_t kHz, uint16_t Hz)
{
    const uint16_t waterfall_width = 100u;
    const uint16_t waterfall_x = 32u;
    const uint16_t waterfall_y = 136u;
    const uint16_t num_cols = 256u;

    static uint8_t waterfall_count = 0u;
    const uint8_t max_waterfall_count = 1u;
    static uint16_t top_row = 0u;

    //draw spectrum scope
    const uint16_t scope_height = 100u;
    const uint16_t scope_x = 32u;
    const uint16_t scope_y = 33u;
    const uint16_t scope_fg = display->colour565(255, 255, 255);

    for(uint16_t col=0; col<num_cols; ++col)
    {
      //scale data point
      uint8_t data_point = (scope_height * (uint16_t)spectrum[col])/318;
      uint16_t vline[scope_height];
  
      const int16_t fbin = col-128;
      const bool is_usb_col = (fbin > fc.start_bin) && (fbin < fc.stop_bin) && fc.upper_sideband;
      const bool is_lsb_col = (-fbin > fc.start_bin) && (-fbin < fc.stop_bin) && fc.lower_sideband;
      const bool is_passband = is_usb_col || is_lsb_col;

      for(uint8_t row=0; row<scope_height; ++row)
      {
        if(row < data_point)
        {
          vline[scope_height - 1 - row] = heatmap((uint16_t)row*256/scope_height, is_passband);
        }
        else if(row == data_point)
        {
          vline[scope_height - 1 - row] = scope_fg;
        }
        else
        {
          vline[scope_height - 1 - row] = heatmap(0, is_passband, fbin==0);
        }
      }
      display->writeVLine(scope_x+col, scope_y, scope_height, vline);
    }

    //draw waterfall
    if(!waterfall_count--)
    {
      waterfall_count = max_waterfall_count;

      //add new line to waterfall
      for(uint16_t col=0; col<num_cols; col++)
      {
        waterfall_buffer[top_row][col] = spectrum[col];
      }
      
      //draw waterfall
      uint16_t line[num_cols];
      for(uint16_t row=0; row<waterfall_width; row++)
      {
        uint16_t row_address = (top_row+row)%waterfall_width;
        for(uint16_t col=0; col<num_cols; ++col)
        {
           const int16_t fbin = col-128;
           const bool is_usb_col = (fbin > fc.start_bin) && (fbin < fc.stop_bin) && fc.upper_sideband;
           const bool is_lsb_col = (-fbin > fc.start_bin) && (-fbin < fc.stop_bin) && fc.lower_sideband;
           const bool is_passband = is_usb_col || is_lsb_col;

           uint8_t heat = waterfall_buffer[row_address][col];
           uint16_t colour=heatmap(heat, is_passband, fbin==0);
           line[col] = colour;
        }
        display->writeHLine(waterfall_x, row+waterfall_y, num_cols, line);
      }

      //scroll waterfall
      top_row = top_row?top_row-1:waterfall_width-1;
    }

    //update frequency
    static uint16_t lastMHz = 0;
    static uint16_t lastkHz = 0;
    static uint16_t lastHz = 0;
    if(lastMHz!=MHz || lastkHz!=kHz || lastHz!=Hz)
    {
      display->fillRect(32, 5, 18, 256, display->colour565(0, 0, 0));
      char frequency[20];
      snprintf(frequency, 20, "%2u.%03u.%03u", MHz, kHz, Hz);
      uint8_t i=0;
      uint8_t x=100;
      while(frequency[i])
      {
        display->drawChar(x, 23, frequency[i], display->colour565(255, 255, 255), &FreeMono12pt7b);
        x+=11;
        i++;
      }
      lastMHz = MHz;
      lastkHz = kHz;
      lastHz = Hz;
    }
}
