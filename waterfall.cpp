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

uint16_t waterfall::heatmap(uint8_t value, bool lighten)
{
    uint8_t section = ((uint16_t)value*6)>>8;
    uint8_t fraction = ((uint16_t)value*6)&0xff;
    uint8_t light_colour = lighten?64:0;

    switch(section)
    {
      case 0: //black->blue
        return display->colour565(light_colour,light_colour,fraction);
      case 1: //blue->cyan
        return display->colour565(light_colour,fraction, 255);
      case 2: //cyan->green
        return display->colour565(light_colour,255, 255-fraction);
      case 3: //green->yellow
        return display->colour565(fraction, 255, light_colour);
      case 4: //yellow->red
        return display->colour565(255, 255-fraction, light_colour);
      case 5: //red->white
        return display->colour565(255, fraction, fraction);
    }
    return 0;
}

void waterfall::new_spectrum(float spectrum[], s_filter_control &fc)
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
    static float min=0.0f;
    static float max=6.0f;

    //normalise spectrum to between 0 and 1
    float new_min = 100.0f;
    float new_max = -100.0f;
    for(uint16_t col=0; col<num_cols; ++col)
    {
      if(spectrum[col]==0) continue;
      new_min = std::fmin(log10f(spectrum[col]), new_min);
      new_max = std::fmax(log10f(spectrum[col]), new_max);
    }
    min=min * 0.9 + new_min * 0.1;
    max=max * 0.9 + new_max * 0.1;
    for(uint16_t col=0; col<num_cols; ++col)
    {
      const float normalised = (log10f(spectrum[col])-min)/(max-min);
      const float clamped = std::fmax(std::fmin(normalised, 1.0f), 0.0f);
      spectrum[col] = clamped;
    }

    //draw spectrum scope
    const uint16_t scope_height = 100u;
    const uint16_t scope_x = 32u;
    const uint16_t scope_y = 18u;
    const uint16_t scope_fg = display->colour565(255, 255, 255);
    const uint16_t scope_bg = display->colour565(0, 0, 0);
    const uint16_t scope_light_bg = display->colour565(64, 64, 64);

    for(uint16_t col=interleave; col<num_cols; col+=2)
    {
      //scale data point
      uint8_t data_point = (scope_height*0.8) * spectrum[col];
      uint16_t vline[scope_height];
  
      const int16_t fbin = col-128;
      const bool is_usb_col = (fbin > fc.start_bin) && (fbin < fc.stop_bin) && fc.upper_sideband;
      const bool is_lsb_col = (-fbin > fc.start_bin) && (-fbin < fc.stop_bin) && fc.lower_sideband;
      const bool is_passband = is_usb_col || is_lsb_col;


      for(uint8_t row=0; row<scope_height; ++row)
      {
        if(fbin == 0)
        {
          vline[scope_height - 1 - row] = scope_fg;
        }
        else if(row < data_point)
        {
          vline[scope_height - 1 - row] = heatmap(row*256/scope_height, is_passband);
        }
        else if(row == data_point)
        {
          vline[scope_height - 1 - row] = scope_fg;
        }
        else
        {
          vline[scope_height - 1 - row] = is_passband?scope_light_bg:scope_bg;
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
        waterfall_buffer[top_row][col] = 255*spectrum[col];
      }
      
      //draw waterfall
      uint16_t line[num_cols];
      for(uint16_t row=0; row<waterfall_width; row++)
      {
        uint16_t row_address = (top_row+row)%waterfall_width;
        for(uint16_t col=0; col<num_cols; ++col)
        {
           uint8_t heat = waterfall_buffer[row_address][col];
           uint16_t colour=heatmap(heat);
           line[col] = colour;
        }
        display->writeHLine(waterfall_x, row+waterfall_y, num_cols, line);
      }

      //scroll waterfall
      top_row = top_row?top_row-1:waterfall_width-1;
    }

    printf("frame rate %f\n", 1.0e6/(time_us_32()-t0));
}
