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
#include "pico/stdlib.h"
#include <cstring>

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#endif
#ifndef __swap_int
#define __swap_int(a, b)   \
    a = (a & b) + (a | b); \
    b = a + (~b) + 1;      \
    a = a + (~b) + 1;
#endif

ILI934X::ILI934X(spi_inst_t *spi, uint8_t cs, uint8_t dc, uint8_t rst, uint16_t width, uint16_t height, ILI934X_ROTATION rotation)
{
    _spi = spi;
    _cs = cs;
    _dc = dc;
    _rst = rst;
    _init_width = _width = width;
    _init_height = _height = height;
    _init_rotation = _rotation = rotation;
}

void ILI934X::reset()
{
    gpio_put(_rst, 0);
    sleep_us(30);
    gpio_put(_rst, 1);
}

void ILI934X::init()
{
    reset();

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

    setRotation(_rotation);

    _write(_PIXSET, (uint8_t *)"\x55", 1);
    _write(_FRMCTR1, (uint8_t *)"\x00\x18", 2);
    _write(_DISCTRL, (uint8_t *)"\x08\x82\x27", 3);
    _write(_ENA3G, (uint8_t *)"\x00", 1);
    _write(_GAMSET, (uint8_t *)"\x01", 1);
    _write(_PGAMCTRL, (uint8_t *)"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00", 15);
    _write(_NGAMCTRL, (uint8_t *)"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f", 15);

    _write(_SLPOUT);
    _write(_DISPON);
}

void ILI934X::setRotation(ILI934X_ROTATION rotation)
{
    uint8_t mode = MADCTL_MX | MADCTL_RGB;
    switch (rotation)
    {
    case R0DEG:
        mode = MADCTL_MX | MADCTL_RGB;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case R90DEG:
        mode = MADCTL_MV | MADCTL_RGB;
        this->_width = this->_init_height;
        this->_height = this->_init_width;
        break;
    case R180DEG:
        mode = MADCTL_MY | MADCTL_RGB;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case R270DEG:
        mode = MADCTL_MY | MADCTL_MX | MADCTL_MV | MADCTL_RGB;
        this->_width = this->_init_height;
        this->_height = this->_init_width;
        break;
    case MIRRORED0DEG:
        mode = MADCTL_MY | MADCTL_MX | MADCTL_RGB;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case MIRRORED90DEG:
        mode = MADCTL_MX | MADCTL_MV | MADCTL_RGB;
        this->_width = this->_init_height;
        this->_height = this->_init_width;
        break;
    case MIRRORED180DEG:
        mode = MADCTL_RGB;
        this->_width = this->_init_width;
        this->_height = this->_init_height;
        break;
    case MIRRORED270DEG:
        mode = MADCTL_MY | MADCTL_MV | MADCTL_RGB;
        this->_width = this->_init_height;
        this->_height = this->_init_width;
        break;
    }

    uint8_t buffer[1] = {mode};
    _write(_MADCTL, (uint8_t *)buffer, 1);
}

void ILI934X::writeHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t line[])
{
    _writeBlock(x, y, x+w-1, y);
    _data((uint8_t *)line, w * 2);
}
void ILI934X::writeVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t line[])
{
    _writeBlock(x, y, x, y+h-1);
    _data((uint8_t *)line, h * 2);
}

void ILI934X::setPixel(uint16_t x, uint16_t y, uint16_t colour)
{
    if (x < 0 || x >= _width || y < 0 || y >= _height)
        return;

    uint16_t buffer[1];
    buffer[0] = colour;

    _writeBlock(x, y, x, y, (uint8_t *)buffer, 2);
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

    uint16_t totalChunks = (uint16_t)((double)(w * h) / _MAX_CHUNK_SIZE);
    uint16_t remaining = (uint16_t)((w * h) % _MAX_CHUNK_SIZE);

    _writeBlock(_x, _y, _x + _w - 1, _y + _h - 1);

    for (uint16_t i = 0; i < totalChunks; i++)
    {
        _data((uint8_t *)buffer, _MAX_CHUNK_SIZE * 2);
    }

    if (remaining > 0)
    {
        _data((uint8_t *)buffer, remaining * 2);
    }
}

void ILI934X::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint16_t _x0 = x0;
    uint16_t _y0 = y0;
    uint16_t _x1 = x1;
    uint16_t _y1 = y1;

    int16_t steep = abs(_y1 - _y0) > abs(_x1 - _x0);
    if (steep)
    {
        __swap_int(_x0, _y0);
        __swap_int(_x1, _y1);
    }

    if (x0 > x1)
    {
        __swap_int(_x0, _x1);
        __swap_int(_y0, _y1);
    }

    int16_t dx, dy;
    dx = _x1 - _x0;
    dy = abs(_y1 - _y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (_y0 < _y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for (; _x0 <= _x1; _x0++)
    {
        if (steep)
        {
            setPixel(_y0, _x0, color);
        }
        else
        {
            setPixel(_x0, _y0, color);
        }
        err -= dy;
        if (err < 0)
        {
            _y0 += ystep;
            err += dx;
        }
    }
}

void ILI934X::drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    setPixel(x0, y0 + r, color);
    setPixel(x0, y0 - r, color);
    setPixel(x0 + r, y0, color);
    setPixel(x0 - r, y0, color);

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        setPixel(x0 + x, y0 + y, color);
        setPixel(x0 - x, y0 + y, color);
        setPixel(x0 + x, y0 - y, color);
        setPixel(x0 - x, y0 - y, color);
        setPixel(x0 + y, y0 + x, color);
        setPixel(x0 - y, y0 + x, color);
        setPixel(x0 + y, y0 - x, color);
        setPixel(x0 - y, y0 - x, color);
    }
}

void ILI934X::blit(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t *bltBuf)
{
    uint16_t _w = MIN(_width - x, MAX(1, w));
    uint16_t _h = MIN(_height - y, MAX(1, h));

    uint16_t remaining = (uint16_t)((w * h) % _MAX_CHUNK_SIZE);
    uint16_t written = 0;

    uint16_t buffer[_MAX_CHUNK_SIZE];

    for (uint16_t iy = 0; iy < _h; iy++)
    {
        for (uint16_t ix = 0; ix < _w; ix++)
        {
            uint16_t idx = ix + iy * w - written;
            if (idx >= _MAX_CHUNK_SIZE)
            {
                _data((uint8_t *)buffer, _MAX_CHUNK_SIZE * 2);
                written += _MAX_CHUNK_SIZE;
                idx -= _MAX_CHUNK_SIZE;
            }

            buffer[idx] = bltBuf[ix + iy * w]; // get pixel from blt buffer
        }
    }

    remaining = w * h - written;

    if (remaining > 0)
    {
        _data((uint8_t *)buffer, remaining * 2);
    }
}

void ILI934X::drawChar(uint16_t x, uint16_t y, char chr, uint16_t colour, const GFXfont *font)
{
    char c = chr - (uint8_t)pgm_read_byte(&font->first);
    GFXglyph *glyph = font->glyph + c;
    uint8_t *bitmap = font->bitmap;
    uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
    uint8_t w = pgm_read_byte(&glyph->width), h = pgm_read_byte(&glyph->height);
    int8_t xo = pgm_read_byte(&glyph->xOffset),
           yo = pgm_read_byte(&glyph->yOffset);
    uint8_t xx, yy, bits = 0, bit = 0;

    for (yy = 0; yy < h; yy++)
    {
        for (xx = 0; xx < w; xx++)
        {
            if (!(bit++ & 7))
            {
                bits = pgm_read_byte(&bitmap[bo++]);
            }
            if (bits & 0x80)
            {
                setPixel(x + xo + xx, y + yo + yy, colour);
            }
            bits <<= 1;
        }
    }
}

void ILI934X::charBounds(char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy, GFXfont *font)
{
    bool wrap = false; // temp no wrap for now
    if (c == '\n')     // Newline?
    {
        *x = 0; // Reset x to zero, advance y by one line
        *y += 1 * (uint8_t)pgm_read_byte(&font->yAdvance);
    }
    else if (c != '\r') // Not a carriage return; is normal char
    {
        uint8_t first = pgm_read_byte(&font->first),
                last = pgm_read_byte(&font->last);
        if ((c >= first) && (c <= last)) // Char present in this font?
        {
            GFXglyph *glyph = font->glyph + (c - first);
            uint8_t gw = pgm_read_byte(&glyph->width),
                    gh = pgm_read_byte(&glyph->height),
                    xa = pgm_read_byte(&glyph->xAdvance);
            int8_t xo = pgm_read_byte(&glyph->xOffset),
                   yo = pgm_read_byte(&glyph->yOffset);

            if (wrap && ((*x + (((int16_t)xo + gw) * 1)) > _width))
            {
                *x = 0; // Reset x to zero, advance y by one line
                *y += 1 * (uint8_t)pgm_read_byte(&font->yAdvance);
            }
            int16_t tsx = (int16_t)1, tsy = (int16_t)1,
                    x1 = *x + xo * tsx, y1 = *y + yo * tsy, x2 = x1 + gw * tsx - 1,
                    y2 = y1 + gh * tsy - 1;
            if (x1 < *minx)
            {
                *minx = x1;
            }
            if (y1 < *miny)
            {
                *miny = y1;
            }
            if (x2 > *maxx)
            {
                *maxx = x2;
            }
            if (y2 > *maxy)
            {
                *maxy = y2;
            }
            *x += xa * tsx;
        }
    }
}

void ILI934X::textBounds(const char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h, GFXfont *font)
{
    uint8_t c;                                                  // Current character
    int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1; // Bound rect
    // Bound rect is intentionally initialized inverted, so 1st char sets it

    *x1 = x; // Initial position is value passed in
    *y1 = y;
    *w = *h = 0; // Initial size is zero

    while ((c = *str++))
    {
        // charBounds() modifies x/y to advance for each character,
        // and min/max x/y are updated to incrementally build bounding rect.
        charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy, font);
    }

    if (maxx >= minx)
    {                         // If legit string bounds were found...
        *x1 = minx;           // Update x1 to least X coord,
        *w = maxx - minx + 1; // And w to bound rect width
    }
    if (maxy >= miny)
    { // Same for height
        *y1 = miny;
        *h = maxy - miny + 1;
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

    // spi write
    uint8_t commandBuffer[1];
    commandBuffer[0] = cmd;

    while (!spi_is_writable(_spi))
    {
        sleep_us(1);
    }

    spi_write_blocking(_spi, commandBuffer, 1);

    gpio_put(_cs, 1);

    // do stuff
    if (data != NULL)
    {
        _data(data, dataLen);
    }
}

void ILI934X::_data(uint8_t *data, size_t dataLen)
{
    gpio_put(_dc, 1);
    gpio_put(_cs, 0);

    spi_write_blocking(_spi, data, dataLen);

    gpio_put(_cs, 1);
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
