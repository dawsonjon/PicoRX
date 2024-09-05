#include "waterfall.h"

#include <cmath>

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
    spi_init(SPI_PORT, 20 * 1000 * 1000);
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
    display->drawLine(32, 119, 288, 119, display->colour565(255,255,255));
}

waterfall::~waterfall()
{
    delete display;
}

void waterfall::heatmap(uint8_t value, uint16_t &colour)
{
    uint8_t section = value / 36;
    uint8_t fraction = (section % 36)*6;

    switch(section)
    {
      case 0: //black->blue
        colour = display->colour565(0,0,fraction);
        return;
      case 1: //blue->cyan
        colour = display->colour565(0,fraction, 255);
        return;
      case 2: //cyan->green
        colour = display->colour565(0,255, 255-fraction);
        return;
      case 3: //green->yellow
        colour = display->colour565(fraction, 255, 0);
        return;
      case 4: //yellow->red
        colour = display->colour565(255, 255-fraction, 0);
        return;
      case 5: //red->white
        colour = display->colour565(255, fraction, fraction);
        return;
    }
}

void waterfall::new_spectrum(float spectrum[])
{
    const uint16_t num_rows = 120u;
    const uint16_t num_cols = 256u;

    static uint8_t waterfall_count = 0;
    static uint16_t top_row = 0;

    if(!waterfall_count--)
    {
      waterfall_count = 4;

      //add new line to waterfall
      for(uint16_t col=0; col<num_cols; col++)
      {
        float min=0.0f;
        float max=6.0f;
        float scale = 256.0f/(max-min);
        waterfall_buffer[top_row][col] = scale*(log10f(spectrum[col])-min);
      }
      
      //draw waterfall
      uint16_t line[320];
      for(uint16_t idx=0; idx<32; ++idx)    line[idx] = display->colour565(0,0,0);
      for(uint16_t idx=288; idx<320; ++idx) line[idx] = display->colour565(0,0,0);
      for(uint16_t row=0; row<120; row++)
      {
        uint16_t row_address = (top_row+row)%num_rows;
        for(uint16_t col=0; col<num_cols; ++col)
        {
           uint8_t heat = waterfall_buffer[row_address][col];
           uint16_t colour=0; 
           heatmap(heat, colour);
           line[32+col] = colour;
        }
        display->writeLine(row+120, 320, line);
      }

      //scroll waterfall
      top_row = top_row?top_row-1:num_rows-1;
    }
}
