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

#include "gfxfont.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#define _SWRST 0x01     // Software Reset
#define _RDDSDR 0x0f    // Read Display Self-Diagnostic Result
#define _SLPOUT 0x11    // Sleep Out
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
#define COLOUR_BLACK 0x0000
#define COLOUR_MAROON 0x8000
#define COLOUR_GREEN 0x0400
#define COLOUR_OLIVE 0x8400
#define COLOUR_NAVY 0x0010
#define COLOUR_PURPLE 0x0801
#define COLOUR_TEAL 0x0410
#define COLOUR_SILVER 0xC618
#define COLOUR_GRAY 0x1084
#define COLOUR_RED 0xF800
#define COLOUR_LIME 0x07E0
#define COLOUR_YELLOW 0xFFE0
#define COLOUR_BLUE 0x001F
#define COLOUR_FUCHSIA 0xF18F
#define COLOUR_AQUA 0x07FF
#define COLOUR_WHITE 0xFFFF

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
    ILI934X(spi_inst_t *spi, uint8_t cs, uint8_t dc, uint8_t rst, uint16_t width = 240, uint16_t height = 320, ILI934X_ROTATION rotation = R0DEG);

    void reset();
    void init();
    void setRotation(ILI934X_ROTATION rotation, bool invert_colours);
    void setPixel(uint16_t x, uint16_t y, uint16_t colour);
    void writeHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t line[]);
    void writeVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t line[]);
    void fillRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour);
    void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    void clear(uint16_t colour = COLOUR_BLACK);
    void drawChar(uint32_t x, uint32_t y, const uint8_t *font, char c, uint16_t fg, uint16_t bg);
    void drawString(uint32_t x, uint32_t y, const uint8_t *font, const char *s, uint16_t fg, uint16_t bg);
    uint16_t colour565(uint8_t r, uint8_t g, uint8_t b);
    void powerOn(bool power_on);
private:
    void _setRotation(ILI934X_ROTATION rotation, bool invert_colours);
    void _write(uint8_t cmd, uint8_t *data = NULL, size_t dataLen = 0);
    void _data(uint8_t *data, size_t dataLen = 0);
    void _writeBlock(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *data = NULL, size_t dataLen = 0);
    uint32_t dma_tx;
    dma_channel_config dma_config;
    
private:
    spi_inst_t *_spi = NULL;
    uint8_t _cs;
    uint8_t _dc;
    uint8_t _rst;
    uint16_t _width;
    uint16_t _height;
    ILI934X_ROTATION _rotation;
    uint16_t _init_width;
    uint16_t _init_height;
    uint8_t _init_rotation;
    uint8_t _invert_colours;
};

#endif //__ILI934X_H__

// https://github.com/jeffmer/micropython-ili9341/blob/master/ili934xnew.py
