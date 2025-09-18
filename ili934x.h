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

#ifndef __ILI934X_H__
#define __ILI934X_H__

#include "hardware/spi.h"
#include "hardware/dma.h"

#define _SWRST 0x01     // Software Reset
#define _RDDSDR 0x0f    // Read Display Self-Diagnostic Result
#define _SLPOUT 0x11    // Sleep Out
#define _INVOFF 0x20    // Inv Off
#define _INVON 0x21     // Inv On
#define _GAMSET 0x26    // Gamma Set
#define _DISPOFF 0x28   // Display Off
#define _DISPON 0x29    // Display On
#define _CASET 0x2a     // Column Address Set
#define _PASET 0x2b     // Page Address Set
#define _RAMWR 0x2c     // Memory Write
#define _RAMRD 0x2e     // Memory Read
#define _MADCTL 0x36    // Memory Access Control
#define _VSCRSADD 0x37  // Vertical Scrolling Start Address
#define _PIXSET 0x3a    // Pixel Format Set
#define _PWCTRLA 0xcb   // Power Control A
#define _PWCRTLB 0xcf   // Power Control B
#define _DTCTRLA 0xe8   // Driver Timing Control A
#define _DTCTRLB 0xea   // Driver Timing Control B
#define _PWRONCTRL 0xed // Power on Sequence Control
#define _PRCTRL 0xf7    // Pump Ratio Control
#define _PWCTRL1 0xc0   // Power Control 1
#define _PWCTRL2 0xc1   // Power Control 2
#define _PWCTRL3 0xc2   // Power Control 3
#define _VMCTRL1 0xc5   // VCOM Control 1
#define _VMCTRL2 0xc7   // VCOM Control 2
#define _FRMCTR1 0xb1   // Frame Rate Control 1
#define _DISCTRL 0xb6   // Display Function Control
#define _ENA3G 0xf2     // Enable 3G
#define _PGAMCTRL 0xe0  // Positive Gamma Control
#define _NGAMCTRL 0xe1  // Negative Gamma Control

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left

#define _MAX_CHUNK_SIZE 256

/* Windows 16 colour pallet converted to 5-6-5 */
const uint16_t COLOUR_BLACK   = __builtin_bswap16(0x0000);
const uint16_t COLOUR_MAROON  = __builtin_bswap16(0x7800);
const uint16_t COLOUR_GREEN   = __builtin_bswap16(0x0400);
const uint16_t COLOUR_OLIVE   = __builtin_bswap16(0x7BE0);
const uint16_t COLOUR_NAVY    = __builtin_bswap16(0x000F);
const uint16_t COLOUR_PURPLE  = __builtin_bswap16(0x8010);
const uint16_t COLOUR_TEAL    = __builtin_bswap16(0x0410);
const uint16_t COLOUR_SILVER  = __builtin_bswap16(0xC618);
const uint16_t COLOUR_GREY    = __builtin_bswap16(0x8410);
const uint16_t COLOUR_RED     = __builtin_bswap16(0xF800);
const uint16_t COLOUR_LIME    = __builtin_bswap16(0x07E0);
const uint16_t COLOUR_YELLOW  = __builtin_bswap16(0xFFE0);
const uint16_t COLOUR_BLUE    = __builtin_bswap16(0x001F);
const uint16_t COLOUR_FUCHSIA = __builtin_bswap16(0xF81F);
const uint16_t COLOUR_AQUA    = __builtin_bswap16(0x07FF);
const uint16_t COLOUR_WHITE   = __builtin_bswap16(0xFFFF);
const uint16_t COLOUR_DARKGREEN   = __builtin_bswap16(0x03E0);
const uint16_t COLOUR_DARKCYAN    = __builtin_bswap16(0x03EF);
const uint16_t COLOUR_LIGHTGREY   = __builtin_bswap16(0xC618);
const uint16_t COLOUR_DARKGREY    = __builtin_bswap16(0x7BEF);
const uint16_t COLOUR_CYAN        = __builtin_bswap16(0x07FF);
const uint16_t COLOUR_MAGENTA     = __builtin_bswap16(0xF81F);
const uint16_t COLOUR_ORANGE      = __builtin_bswap16(0xFD20);
const uint16_t COLOUR_GREENYELLOW = __builtin_bswap16(0xAFE5);
const uint16_t COLOUR_PINK        = __builtin_bswap16(0xF81F);

enum e_display_type{
  ILI9341,
  ILI9341_2,
  ILI9488,
  ST7796
};


enum ILI934X_ROTATION
{
    R0DEG,
    R90DEG,
    R180DEG,
    R270DEG,
    MIRRORED0DEG,
    MIRRORED90DEG,
    MIRRORED180DEG,
    MIRRORED270DEG
};

class ILI934X
{
public:
    ILI934X(spi_inst_t *spi, uint8_t cs, uint8_t dc, uint16_t width = 240, uint16_t height = 320);

    void init(ILI934X_ROTATION rotation, bool invert_colours, bool invert_display, e_display_type display_type);
    void configure_ili934x();
    void configure_st7796();
    void configure_ili9488();
    void configure_ili9341_2();
    void setPixel(uint16_t x, uint16_t y, uint16_t colour);
    void writeHLine(uint16_t x, uint16_t y, uint16_t w, const uint16_t line[]);
    void writeVLine(uint16_t x, uint16_t y, uint16_t h, const uint16_t line[]);
    void writeImage(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, const uint16_t *data);
    void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    void drawFastHline(int x0, int x1, int y, uint16_t colour);
    void drawFastVline(int x, int y0, int y1, uint16_t colour);
    void fillCircle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour);
    void drawCircle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour);
    void drawRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour);
    void fillRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour);
    void fillRoundedRect(int x, int y, int h, int w, int r, uint16_t colour);
    void drawRoundedRect(int x, int y, int h, int w, int r, uint16_t colour);
    void drawEllipse(int xc, int yc, int rx, int ry, uint16_t color); 
    void fillEllipse(int xc, int yc, int rx, int ry, uint16_t color);
    void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t colour);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t colour);
    void clear(uint16_t colour = COLOUR_BLACK);
    void drawChar(uint32_t x, uint32_t y, const uint8_t *font, char c, uint16_t fg, uint16_t bg);
    void drawString(uint32_t x, uint32_t y, const uint8_t *font, const char *s, uint16_t fg, uint16_t bg);
    uint16_t colour565(uint8_t r, uint8_t g, uint8_t b);
    void powerOn(bool power_on);
    void dmaData(uint8_t *data, size_t dataLen);
    void dmaFlush();

private:
    e_display_type _display_type;
    void drawCircleQuadrant(int xc, int yc, int r, int quadrant, uint16_t colour);
    void drawFilledCircleQuadrant(int xc, int yc, int r, int quadrant, uint16_t colour);
    void _writeBlock(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *data = NULL, size_t dataLen = 0);
    void _setRotation(ILI934X_ROTATION rotation, bool invert_colours);
    void _write(uint8_t cmd, uint8_t *data = NULL, size_t dataLen = 0);
    void _writePixels(const uint16_t *data, size_t dataLen);
    void _data(uint8_t *data, size_t dataLen = 0);
    uint32_t dma_tx;
    dma_channel_config dma_config;
    
private:
    spi_inst_t *_spi = NULL;
    uint8_t _cs;
    uint8_t _dc;
    uint8_t _rst;
    uint16_t _width;
    uint16_t _height;
    uint16_t _init_width;
    uint16_t _init_height;
};

#endif //__ILI934X_H__
