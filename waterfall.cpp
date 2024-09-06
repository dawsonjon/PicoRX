#include "waterfall.h"

#include <cmath>
#include <cstdio>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ili934x.h"

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
    spi_init(SPI_PORT, 40 * 1000 * 1000);
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
    display->drawLine(32, 119, 287, 119, display->colour565(255,255,255));
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

void waterfall::new_spectrum(uint8_t spectrum[], s_filter_control &fc)
{

    uint32_t t0 = time_us_32();
    const uint16_t waterfall_width = 100u;
    const uint16_t waterfall_x = 32u;
    const uint16_t waterfall_y = 120u;
    const uint16_t num_cols = 256u;

    static uint8_t waterfall_count = 0u;
    const uint8_t max_waterfall_count = 1u;
    static uint16_t top_row = 0u;
    static uint8_t interleave = false;

    //draw spectrum scope
    const uint16_t scope_height = 100u;
    const uint16_t scope_x = 32u;
    const uint16_t scope_y = 18u;
    const uint16_t scope_fg = display->colour565(255, 255, 255);

    for(uint16_t col=interleave; col<num_cols; col+=2)
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
    interleave^=1;

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

    printf("frame rate %f\n", 1.0e6/(time_us_32()-t0));
}
