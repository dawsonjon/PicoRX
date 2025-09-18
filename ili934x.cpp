/*
BSD 3-Clause License

Copyright (c) 2022, Darren Horrocks
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdlib.h"
#include "stdio.h"
#include "ili934x.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"
#include <cstring>
#include <algorithm>

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#endif

ILI934X::ILI934X(spi_inst_t *spi, uint8_t cs, uint8_t dc, uint16_t width, uint16_t height)
{
    _spi = spi;
    _cs = cs;
    _dc = dc;
    _init_width = width;
    _init_height = height;

    dma_tx = dma_claim_unused_channel(true);
    dma_config = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_8);
    channel_config_set_dreq(&dma_config, spi_get_dreq(_spi, true));
    channel_config_set_read_increment(&dma_config, true);
    channel_config_set_write_increment(&dma_config, false);
}

void ILI934X::configure_ili9488()
{

  // Positive Gamma Control
  _write(0xE0, (uint8_t *)"\x00\x03\x09\x08\x16\x0A\x3F\x78\x4C\x09\x0A\x8\x16\x1A\x0F", 15);

  // Negative Gamma Control
  _write(0XE1, (uint8_t *)"\x00\x16\x19\x03\x0F\x05\x32\x45\x46\x04\x0E\x0D\x35\x37\x0F", 15);

  // Power Control 1
  _write(0XC0, (uint8_t *)"\x17\x15", 2);

  // Power Control 2
  _write(0xC1, (uint8_t *)"\x41", 1);

  // VCOM Control
  _write(0xC5, (uint8_t *)"\x00\x12\x80", 3);
  
  // Power Control 2
  _write(_MADCTL, (uint8_t *)"\x48", 1);

  // Pixel Interface Format
  _write(0x3A, (uint8_t *)"\x66", 1);

  // Interface Mode Control
  _write(0xB0, (uint8_t *)"\x00", 1);

  // Frame Rate Control
  _write(0xB1, (uint8_t *)"\xA0", 1);

  // Display Inversion Control
  _write(0xB4, (uint8_t *)"\x02", 1);

  // Display Function Control
  _write(0xB6, (uint8_t *)"\x02\x02\x3B", 3);

  // Entry Mode Set
  _write(0xB7, (uint8_t *)"\xC6", 1);

  // Adjust Control 3
  _write(0xF7, (uint8_t *)"\xA9\x51\x2C\x82", 4);

  //Exit Sleep
  _write(_SLPOUT, NULL, 0);
  sleep_ms(120);

  //Display on
  _write(_DISPON, NULL, 0);  
  sleep_ms(25);
}

void ILI934X::configure_st7796()
{
  sleep_ms(120);
  _write(0x01, NULL, 0);
  sleep_ms(120);
  _write(0x11, NULL, 0);
  sleep_ms(120);
  
  //Command Set control
  _write(0xF0, (uint8_t *)"\xC3", 1);//Enable extension command 2 partI                               
  _write(0xF0, (uint8_t *)"\x96", 1);//Enable extension command 2 partII                               
  _write(0x36, (uint8_t *)"\x48", 1);

  //Interface Pixel Format
  _write(_PIXSET, (uint8_t *)"\x55", 1);//Control interface color format set to 16 
	
  //Column inversion
  _write(0xB4, (uint8_t *)"\x01", 1);//1-dot inversion 

  //Display Function Control
  _write(_DISCTRL, (uint8_t *)"\x80\x02\x3B", 3);//1-dot inversion 

  //Display Output Ctrl Adjust
  _write(_DTCTRLA, (uint8_t *)"\x40\x8A\x00\x00\x29\x19\xA5\x33", 8);
	
  //Power control2
  _write(_PWCTRL2, (uint8_t *)"\x06", 1);

  //Power control 3
  _write(_PWCTRL3, (uint8_t *)"\xA7", 1); 

  //VCOM Control
  _write(_VMCTRL1, (uint8_t *)"\x18", 1);

  sleep_ms(120);
	
  //ST7796 Gamma Sequence
  _write(_PGAMCTRL, (uint8_t *)"\xF0\x09\x0b\x06\x04\x15\x2f\x54\x42\x3c\x17\x14\x18\x1b", 14); 
  _write(_NGAMCTRL, (uint8_t *)"\xE0\x09\x0B\x06\x04\x03\x2B\x43\x42\x3B\x16\x14\x17\x1B", 14);

  sleep_ms(120);
	
  //Command Set control
  _write(0xF0, (uint8_t *)"\x3C", 1);
  _write(0xF0, (uint8_t *)"\x69", 1);

  sleep_ms(120);

  _write(0x29, NULL, 0);
}

void ILI934X::configure_ili934x()
{
  _write(_RDDSDR, (uint8_t *)"\x03\x80\x02", 3);
  _write(_PWCRTLB, (uint8_t *)"\x00\xc1\x30", 3);
  _write(_PWRONCTRL, (uint8_t *)"\x64\x03\x12\x81", 4);
  _write(_DTCTRLA, (uint8_t *)"\x85\x00\x78", 3);
  _write(_PWCTRLA, (uint8_t *)"\x39\x2c\x00\x34\x02", 5);
  _write(_PRCTRL, (uint8_t *)"\x20", 1);
  _write(_DTCTRLB, (uint8_t *)"\x00\x00", 2);
  _write(_PWCTRL1, (uint8_t *)"\x23", 1);
  _write(_PWCTRL2, (uint8_t *)"\x10", 1);
  _write(_VMCTRL1, (uint8_t *)"\x3e\x28", 2);
  _write(_VMCTRL2, (uint8_t *)"\x86", 1);

  _write(_PIXSET, (uint8_t *)"\x55", 1);
  _write(_FRMCTR1, (uint8_t *)"\x00\x18", 2);
  _write(_DISCTRL, (uint8_t *)"\x08\x82\x27", 3);
  _write(_ENA3G, (uint8_t *)"\x00", 1);
  _write(_GAMSET, (uint8_t *)"\x01", 1);
  _write(_PGAMCTRL, (uint8_t *)"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00", 15);
  _write(_NGAMCTRL, (uint8_t *)"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f", 15);

  //Exit Sleep
  _write(_SLPOUT, NULL, 0);

  sleep_ms(120);

  //Display on
  _write(_DISPON, NULL, 0);

}

void ILI934X::configure_ili9341_2()
{
  _write(0xEF, (uint8_t*)"\x03\x80\x02", 3);
  _write(0xCF, (uint8_t*)"\x00\xC1\x30", 3);
  _write(0xED, (uint8_t*)"\x64\x03\x12\x81", 4);
  _write(0xE8, (uint8_t*)"\x85\x00\x78", 3);
  _write(0xCB, (uint8_t*)"\x39\x2C\x00\x34\x02", 5);
  _write(0xF7, (uint8_t*)"\x20", 1);
  _write(0xEA, (uint8_t*)"\x00\x00", 2);

  //Power control
  _write(_PWCTRL1, (uint8_t*)"\x23, 1");

  //Power control
  _write(_PWCTRL2, (uint8_t*)"\x10, 1");

  //VCM control
  _write(_VMCTRL1, (uint8_t*)"\x3e\x28", 2);

  //VCM control2
  _write(_VMCTRL2, (uint8_t*)"\x86", 1);

  // Memory Access Control
  _write(_MADCTL, (uint8_t*)"\x40", 1);
  _write(_PIXSET, (uint8_t*)"\x55", 1);
  _write(_FRMCTR1, (uint8_t*)"\x00\x13", 2);

  // Display Function Control
  _write(_DISCTRL, (uint8_t*)"\x08\x82\x27", 3);

  // 3Gamma Function Disable
  _write(0xF2, (uint8_t*)"\x00", 1);

   //Gamma curve selected
  _write(_GAMSET, (uint8_t*)"\x01", 1);

   //Set Gamma
  _write(_PGAMCTRL, (uint8_t*)"\x0F\x31\x2B\x0C\x0E\x08\x4E\xF1\x37\x07\x10\x03\x0E\x09\x00", 15);

  //Set Gamma
  _write(_NGAMCTRL, (uint8_t*)"\x00\x0E\x14\x03\x11\x07\x31\xC1\x48\x08\x0F\x0C\x31\x36\x0F", 15);

  //Exit Sleep
  _write(_SLPOUT, NULL, 0);

  sleep_ms(120);

  //Display on
  _write(_DISPON, NULL, 0);
}

void ILI934X::init(ILI934X_ROTATION rotation, bool invert_colours, bool invert_display, e_display_type display_type)
{
  sleep_ms(5);
  _write(_SWRST, NULL, 0);
	sleep_ms(150);

  if(display_type == ILI9341)
  {
    configure_ili934x();
  }
  else if(display_type == ST7796)
  {
    configure_st7796();
  }
  else if(display_type == ILI9488)
  {
    configure_ili9488();
  }
  else if(display_type == ILI9341_2)
  {
    configure_ili9341_2();
  }

  if (invert_display) 
  {
    _write(_INVON);
  }
  else
  {
    _write(_INVOFF);
  }

  _setRotation(rotation, invert_colours);
  _display_type = display_type;

}

void ILI934X::powerOn(bool power_on)
{
  if(power_on)
  {
    _write(_DISPON, NULL, 0);
  }
  else
  {
    _write(_DISPOFF, NULL, 0);
  }
}

void ILI934X::_setRotation(ILI934X_ROTATION rotation, bool invert_colours)
{

    uint8_t colour_flags = invert_colours?MADCTL_BGR:MADCTL_RGB;
    uint8_t mode = MADCTL_MX | colour_flags;
    switch (rotation)
    {
    case R0DEG:
        mode = MADCTL_MX | colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case R90DEG:
        mode = MADCTL_MV | colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case R180DEG:
        mode = MADCTL_MY | colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case R270DEG:
        mode = MADCTL_MY | MADCTL_MX | MADCTL_MV | colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case MIRRORED0DEG:
        mode = MADCTL_MY | MADCTL_MX | colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case MIRRORED90DEG:
        mode = MADCTL_MX | MADCTL_MV | colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case MIRRORED180DEG:
        mode = colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case MIRRORED270DEG:
        mode = MADCTL_MY | MADCTL_MV | colour_flags;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    }

    uint8_t buffer[1] = {mode};
    _write(_MADCTL, (uint8_t *)buffer, 1);
}

void ILI934X::writeHLine(uint16_t x, uint16_t y, uint16_t w, const uint16_t line[])
{
    _writeBlock(x, y, x+w-1, y);
    dmaData((uint8_t *)line, w * 2);
}
void ILI934X::writeVLine(uint16_t x, uint16_t y, uint16_t h, const uint16_t line[])
{
    _writeBlock(x, y, x, y+h-1);
    dmaData((uint8_t *)line, h * 2);
}

void ILI934X::setPixel(uint16_t x, uint16_t y, uint16_t colour)
{
    if (x < 0 || x >= _width || y < 0 || y >= _height)
        return;

    uint16_t buffer[1];
    buffer[0] = colour;

    _writeBlock(x, y, x, y);
    _writePixels(buffer, 1);
}

void ILI934X::fillRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour)
{
    uint16_t _x = MIN(_width - 1, MAX(0, x));
    uint16_t _y = MIN(_height - 1, MAX(0, y));
    uint16_t _w = MIN(_width - x, MAX(1, w));
    uint16_t _h = MIN(_height - y, MAX(1, h));

    uint16_t buffer[_MAX_CHUNK_SIZE];
    for (int x = 0; x < _MAX_CHUNK_SIZE; x++)
    {
        buffer[x] = colour;
    }

    uint16_t totalChunks = ((uint32_t)w * h) / _MAX_CHUNK_SIZE;
    uint16_t remaining = ((uint32_t)w * h) % _MAX_CHUNK_SIZE;

    _writeBlock(_x, _y, _x + _w - 1, _y + _h - 1);

    for (uint16_t i = 0; i < totalChunks; i++)
    {
        _writePixels((uint16_t *)buffer, _MAX_CHUNK_SIZE);
    }

    if (remaining > 0)
    {
        _writePixels((uint16_t *)buffer, remaining);
    }

    
}

void ILI934X::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;
  
  while (1) {
      setPixel(x0, y0, color);
      if (x0 == x1 && y0 == y1) break;
      e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void ILI934X::drawChar(uint32_t x, uint32_t y, const uint8_t *font, char c, uint16_t fg, uint16_t bg) {

    const uint8_t font_height = font[0];
    const uint8_t font_width  = font[1];
    const uint8_t font_space  = font[2];
    const uint8_t first_char  = font[3];
    const uint8_t last_char   = font[4];
    const uint16_t bytes_per_char = (font_width*font_height+4)/8;
    if(c<first_char||c>last_char) return;

    uint16_t buffer[font_height][font_width+font_space];
    uint16_t font_index = ((c-first_char)*bytes_per_char) + 5u;
    uint8_t data = font[font_index++];
    uint8_t bits_left = 8;

    for(uint8_t xx = 0; xx<font_width; ++xx)
    {
      for(uint8_t yy = 0; yy<font_height; ++yy)
      {
        if(data & 0x01)
        {
          buffer[yy][xx] = fg;
        }
        else
        {
          buffer[yy][xx] = bg;
        }
        data >>= 1;
        bits_left--;
        if(bits_left == 0)
        {
          data = font[font_index++];
          bits_left = 8;
        }
      }
    }
    for(uint8_t xx = 0; xx<font_space; ++xx)
    {
      for(uint8_t yy = 0; yy<font_height; ++yy)
      {
        buffer[yy][font_width+xx] = bg;
      }
    }

    _writeBlock(x, y, x+font_width+font_space-1, y+font_height-1);
    _writePixels((uint16_t *)buffer, sizeof(buffer)/2);
}

void ILI934X::drawString(uint32_t x, uint32_t y, const uint8_t *font, const char *s, uint16_t fg, uint16_t bg) {
    const uint8_t font_width  = font[1];
    const uint8_t font_space  = font[2];
    for(int32_t x_n=x; *s; x_n+=(font_width+font_space)) {
        drawChar(x_n, y, font, *(s++), fg, bg);
    }
}

void ILI934X::clear(uint16_t colour)
{
    fillRect(0, 0, _height, _width, colour);
}

void ILI934X::_write(uint8_t cmd, uint8_t *data, size_t dataLen)
{
    gpio_put(_dc, 0);
    gpio_put(_cs, 0);
    sleep_us(1);

    // spi write
    uint8_t commandBuffer[1];
    commandBuffer[0] = cmd;
    spi_write_blocking(_spi, commandBuffer, 1);


    // do stuff
    if (data != NULL)
    {
        _data(data, dataLen);
    }
    else
    {
        sleep_us(1);
        gpio_put(_cs, 1);
    }


}

void ILI934X::_data(uint8_t *data, size_t dataLen)
{

    gpio_put(_dc, 1);
    gpio_put(_cs, 0);
    sleep_us(1);
    spi_write_blocking(_spi, data, dataLen);
    sleep_us(1);
    gpio_put(_cs, 1);

}

void ILI934X::_writePixels(const uint16_t *data, size_t numPixels)
{
  if(_display_type == ILI9488)
  {
    uint32_t pixelIndex = 0;
    while(numPixels)
    {
      const uint32_t chunkSize = std::min((uint32_t)numPixels, (uint32_t)256);
      uint8_t new_data[256][3];
      for(uint32_t idx=0; idx<chunkSize; idx++)
      {
        uint16_t pixel = __builtin_bswap16(data[pixelIndex++]);
        new_data[idx][0] = (pixel & 0xf800) >> 8;
        new_data[idx][1] = (pixel & 0x07e0) >> 3;
        new_data[idx][2] = (pixel & 0x001F) << 3;
      }
      _data( (uint8_t*)new_data, 3*chunkSize);

      numPixels -= chunkSize;
    }
  }
  else
  {
    _data( (uint8_t*)data, 2*numPixels);
  }
}


void ILI934X::_writeBlock(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *data, size_t dataLen)
{
    uint16_t buffer[2];
    buffer[0] = __builtin_bswap16(x0);
    buffer[1] = __builtin_bswap16(x1);

    _write(_CASET, (uint8_t *)buffer, 4);

    buffer[0] = __builtin_bswap16(y0);
    buffer[1] = __builtin_bswap16(y1);

    _write(_PASET, (uint8_t *)buffer, 4);
    _write(_RAMWR, data, dataLen);
}

uint16_t ILI934X::colour565(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t val = (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f);
    return __builtin_bswap16(val);
}

void ILI934X::writeImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
  _writeBlock(x, y, x+w-1, y+h-1);
  _writePixels(data, w*h);
}

void ILI934X::fillCircle(uint16_t xc, uint16_t yc, uint16_t r, uint16_t colour)
{
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        // Draw horizontal spans between symmetric points
        drawFastHline(xc - x, xc + x, yc + y, colour);
        drawFastHline(xc - x, xc + x, yc - y, colour);
        drawFastHline(xc - y, xc + y, yc + x, colour);
        drawFastHline(xc - y, xc + y, yc - x, colour);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void ILI934X:: drawCircle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour) 
{
    int16_t x = 0, y = radius;
    int16_t d = 1 - radius;
    
    while (x <= y) {
        setPixel(xc + x, yc + y, colour);
        setPixel(xc - x, yc + y, colour);
        setPixel(xc + x, yc - y, colour);
        setPixel(xc - x, yc - y, colour);
        setPixel(xc + y, yc + x, colour);
        setPixel(xc - y, yc + x, colour);
        setPixel(xc + y, yc - x, colour);
        setPixel(xc - y, yc - x, colour);
        
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
}

void ILI934X::drawRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour)
{
  fillRect(x, y, 1, w, colour);
  fillRect(x, y+h-1, 1, w, colour);
  fillRect(x, y, h, 1, colour);
  fillRect(x+w-1, y, h, 1, colour);
}

void ILI934X::drawEllipse(int xc, int yc, int rx, int ry, uint16_t colour) 
{
    int x = 0;
    int y = ry;
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int two_rx2 = 2 * rx2;
    int two_ry2 = 2 * ry2;
    int px = 0;
    int py = two_rx2 * y;

    // Region 1
    int p1 = ry2 - (rx2 * ry) + (0.25 * rx2);
    while (px < py) {
        // Draw symmetry points
        setPixel(xc + x, yc + y, colour);
        setPixel(xc - x, yc + y, colour);
        setPixel(xc + x, yc - y, colour);
        setPixel(xc - x, yc - y, colour);

        x++;
        px += two_ry2;
        if (p1 < 0) {
            p1 += ry2 + px;
        } else {
            y--;
            py -= two_rx2;
            p1 += ry2 + px - py;
        }
    }

    // Region 2
    int p2 = ry2 * (x + 0.5f) * (x + 0.5f) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    while (y >= 0) {
        setPixel(xc + x, yc + y, colour);
        setPixel(xc - x, yc + y, colour);
        setPixel(xc + x, yc - y, colour);
        setPixel(xc - x, yc - y, colour);

        y--;
        py -= two_rx2;
        if (p2 > 0) {
            p2 += rx2 - py;
        } else {
            x++;
            px += two_ry2;
            p2 += rx2 - py + px;
        }
    }
}

void ILI934X:: drawFastHline(int x0, int x1, int y, uint16_t colour)
{
  fillRect(std::min(x0, x1), y, 1, std::max(x0, x1) - std::min(x0, x1) + 1, colour);
}

void ILI934X:: drawFastVline(int x, int y0, int y1, uint16_t colour)
{
  fillRect(x, std::min(y0, y1), std::max(y0, y1) - std::min(y0, y1) + 1, 1, colour);
}

void ILI934X:: fillEllipse(int xc, int yc, int rx, int ry, uint16_t colour) 
{
    int x = 0;
    int y = ry;
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int two_rx2 = 2 * rx2;
    int two_ry2 = 2 * ry2;
    int px = 0;
    int py = two_rx2 * y;

    // Region 1
    int p1 = ry2 - (rx2 * ry) + (0.25 * rx2);
    while (px < py) {
        drawFastHline(xc - x, xc + x, yc + y, colour);
        drawFastHline(xc - x, xc + x, yc - y, colour);

        x++;
        px += two_ry2;
        if (p1 < 0) {
            p1 += ry2 + px;
        } else {
            y--;
            py -= two_rx2;
            p1 += ry2 + px - py;
        }
    }

    // Region 2
    int p2 = ry2 * (x + 0.5f) * (x + 0.5f) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    while (y >= 0) {
        drawFastHline(xc - x, xc + x, yc + y, colour);
        drawFastHline(xc - x, xc + x, yc - y, colour);

        y--;
        py -= two_rx2;
        if (p2 > 0) {
            p2 += rx2 - py;
        } else {
            x++;
            px += two_ry2;
            p2 += rx2 - py + px;
        }
    }
}

void ILI934X:: drawFilledCircleQuadrant(int xc, int yc, int r, int quadrant, uint16_t colour) 
{
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        // Draw only one quarter depending on quadrant
        switch (quadrant) {
            case 0: // Top-left
                fillRect(xc - y, yc - x, 1, y + 1, colour);
                fillRect(xc - x, yc - y, 1, x + 1, colour);
                break;
            case 1: // Top-right
                fillRect(xc, yc - x, 1, y + 1, colour);
                fillRect(xc, yc - y, 1, x + 1, colour);
                break;
            case 2: // Bottom-left
                fillRect(xc - y, yc + x, 1, y + 1, colour);
                fillRect(xc - x, yc + y, 1, x + 1, colour);
                break;
            case 3: // Bottom-right
                fillRect(xc, yc + x, 1, y + 1, colour);
                fillRect(xc, yc + y, 1, x + 1, colour);
                break;
        }

        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void ILI934X:: fillRoundedRect(int x, int y, int h, int w, int r, uint16_t colour) 
{
    // Sanity check: radius can't be more than half width/height
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    // 1. Draw center rectangle (main body)
    fillRect(x+r, y, r, w-(2*r), colour);
    fillRect(x, y+r, h-(2*r), w, colour);
    fillRect(x+r, y+h-r, r, w-(2*r), colour);

    // 2. Draw rounded corners (quarter circles)
    drawFilledCircleQuadrant(x + r,     y + r,     r, 0, colour); // Top-left
    drawFilledCircleQuadrant(x + w - r - 1, y + r,     r, 1, colour); // Top-right
    drawFilledCircleQuadrant(x + r,     y + h - r - 1, r, 2, colour); // Bottom-left
    drawFilledCircleQuadrant(x + w - r - 1, y + h - r - 1, r, 3, colour); // Bottom-right
}

void ILI934X:: drawCircleQuadrant(int xc, int yc, int r, int quadrant, uint16_t colour) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        switch (quadrant) {
            case 0: // Top-left
                setPixel(xc - x, yc - y, colour);
                setPixel(xc - y, yc - x, colour);
                break;
            case 1: // Top-right
                setPixel(xc + x, yc - y, colour);
                setPixel(xc + y, yc - x, colour);
                break;
            case 2: // Bottom-left
                setPixel(xc - x, yc + y, colour);
                setPixel(xc - y, yc + x, colour);
                break;
            case 3: // Bottom-right
                setPixel(xc + x, yc + y, colour);
                setPixel(xc + y, yc + x, colour);
                break;
        }

        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void ILI934X:: drawRoundedRect(int x, int y, int h, int w, int r, uint16_t colour) {
    // Limit radius
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    // 1. Straight sides
    fillRect(x + r, y, 1, w - 2 * r, colour);                 // Top
    fillRect(x + r, y + h - 1, 1, w - 2 * r, colour);         // Bottom
    fillRect(x,     y + r, h - 2 * r, 1, colour);             // Left
    fillRect(x + w - 1, y + r, h - 2 * r, 1, colour);         // Right

    // 2. Rounded corners
    drawCircleQuadrant(x + r,         y + r,         r, 0, colour); // Top-left
    drawCircleQuadrant(x + w - r - 1, y + r,         r, 1, colour); // Top-right
    drawCircleQuadrant(x + r,         y + h - r - 1, r, 2, colour); // Bottom-left
    drawCircleQuadrant(x + w - r - 1, y + h - r - 1, r, 3, colour); // Bottom-right
}

void ILI934X:: drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t colour) 
{
    drawLine(x0, y0, x1, y1, colour);
    drawLine(x1, y1, x2, y2, colour);
    drawLine(x2, y2, x0, y0, colour);
}

void ILI934X:: fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t colour) 
{
    // Sort the vertices by y-coordinate ascending (y0 <= y1 <= y2)
    if (y0 > y1) { int t; t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y1 > y2) { int t; t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y0 > y1) { int t; t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }

    int total_height = y2 - y0;

    for (int y = y0; y <= y2; y++) {
        int second_half = y > y1 || y1 == y0;
        int segment_height = second_half ? y2 - y1 : y1 - y0;
        if (segment_height == 0) continue;

        float alpha = (float)(y - y0) / total_height;
        float beta  = (float)(y - (second_half ? y1 : y0)) / segment_height;

        int ax = x0 + (x2 - x0) * alpha;
        int bx = second_half
            ? x1 + (x2 - x1) * beta
            : x0 + (x1 - x0) * beta;

        if (ax > bx) { int t = ax; ax = bx; bx = t; }
        drawFastHline(ax, bx, y, colour);
    }
}

void ILI934X::dmaData(uint8_t *data, size_t dataLen)
{
    gpio_put(_dc, 1);
    gpio_put(_cs, 0);
    sleep_us(5);

    dma_channel_configure(dma_tx, &dma_config,
                          &spi_get_hw(_spi)->dr,
                          data,
                          dataLen,
                          true);
}

void ILI934X::dmaFlush()
{
    dma_channel_wait_for_finish_blocking(dma_tx);
    sleep_us(5);
    gpio_put(_cs, 1);
}

