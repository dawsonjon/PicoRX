/*

MIT License

Copyright (c) 2021 David Schramm

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <pico/binary_info.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ssd1306.h"
#include "font_8x5.h"

inline static void swap(int32_t *a, int32_t *b) {
    int32_t *t=a;
    *a=*b;
    *b=*t;
}

inline static void fancy_write(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, char *name) {
    switch(i2c_write_blocking(i2c, addr, src, len, false)) {
    case PICO_ERROR_GENERIC:
        printf("[%s] addr not acknowledged!\n", name);
        break;
    case PICO_ERROR_TIMEOUT:
        printf("[%s] timeout!\n", name);
        break;
    default:
        //printf("[%s] wrote successfully %lu bytes!\n", name, len);
        break;
    }
}

inline static void ssd1306_write(ssd1306_t *p, uint8_t val) {
    uint8_t d[2]= {0x00, val};
    fancy_write(p->i2c_i, p->address, d, 2, "ssd1306_write");
}

 void ssd1306_set_start_line(ssd1306_t *p, uint8_t val) {
    ssd1306_write(p, 0x40+val);
 }

void ssd1306_scroll_screen(ssd1306_t *p, int16_t x, int16_t y) {
  // index 0 of each page has a fixed 0x40 header for I2C
  // so we run from 1..128 (or 128..1)
  int32_t start = 1;
  int32_t end = p->width+1;
  int32_t count = 1;
  if (x>0) { // run loop from right to left
    start = p->width;
    end = 0;
    count = -1;
  }
  for (int32_t i=start; i!=end; i+=count) {
    // assemble 8 bytes of a column into 64 bit number
    uint64_t temp=0;
    for (uint32_t page=0; page < p->pages; page++) {
      temp |= (uint64_t)p->buffer[page*(p->width+1) + i] << 8*page;
    }
    // shift it
    if (y>0) {
        temp <<= y;
    } else if (y<0) {
        temp >>= -y;
    }
    // split it back out
    // can do some X shifting here too...
    unsigned int new_x = i + x;
    if ((new_x >= 1) && (new_x <= p->width)) {
        for (uint32_t page=0; page < p->pages; page++) {
            p->buffer[page*(p->width+1) + new_x] = temp & 0xff;
            temp >>= 8;
        }
    }
  }
  if (x != 0) {
    // assume x is negative so we zero from the right
    int32_t start = p->width+1+x;
    int32_t end = p->width;
    if (x>0) { // zero from the left
        start = 1;
        end = x;
    }
    for (int32_t i=start; i<=end; i++) {
        for (uint32_t page=0; page < p->pages; page++) {
            p->buffer[page*(p->width+1) + i] = 0;
        }
    }
  }
}

bool ssd1306_init(ssd1306_t *p, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance) {
    p->width=width;
    p->height=height;
    p->pages=height/8;
    p->address=address;
    p->disp_col_offset=0;
    p->i2c_i=i2c_instance;
    p->curr_page=0;
    p->curr_buf=0;
    p->show_pending = false;

    // 1 + p->width so each page is 129 bytes long
    // allows room for the 0x40 byte at the start of each page
    // draw_pixel knows about this and does the right thing
    p->bufsize = (p->pages) * (1 + p->width);
    if((p->mem=malloc(2 * p->bufsize))==NULL) {
        p->bufsize=0;
        return false;
    }
    p->buffer = &p->mem[p->bufsize * p->curr_buf];

    // from https://github.com/makerportal/rpi-pico-ssd1306
    uint8_t cmds[]= {
        SET_DISP,
        // timing and driving scheme
        SET_DISP_CLK_DIV,
        0x80,
        SET_MUX_RATIO,
        height - 1,
        SET_DISP_OFFSET,
        0x00,
        // resolution and layout
        SET_DISP_START_LINE,
        // charge pump
        SET_CHARGE_PUMP,
        p->external_vcc?0x10:0x14,
        SET_SEG_REMAP | 0x01,           // column addr 127 mapped to SEG0
        SET_COM_OUT_DIR | 0x08,         // scan from COM[N] to COM0
        SET_COM_PIN_CFG,
        width>2*height?0x02:0x12,
        // display
        SET_CONTRAST,
        0xff,
        SET_PRECHARGE,
        p->external_vcc?0x22:0xF1,
        SET_VCOM_DESEL,
        0x30,                           // or 0x40?
        SET_ENTIRE_ON,                  // output follows RAM contents
        SET_NORM_INV,                   // not inverted
        SET_DISP | 0x01,
        // address setting
        SET_MEM_ADDR,
        0x00,  // horizontal
    };

    for(size_t i=0; i<sizeof(cmds); ++i)
        ssd1306_write(p, cmds[i]);

    return true;
}

inline void ssd1306_deinit(ssd1306_t *p)
{
    if ( p->mem )
    {
        free(p->mem);
    }
}

inline void ssd1306_poweroff(ssd1306_t *p) {
    ssd1306_write(p, SET_DISP|0x00);
}

inline void ssd1306_poweron(ssd1306_t *p) {
    ssd1306_write(p, SET_DISP|0x01);
}

inline void ssd1306_contrast(ssd1306_t *p, uint8_t val) {
    ssd1306_write(p, SET_CONTRAST);
    ssd1306_write(p, val);
}

inline void ssd1306_invert(ssd1306_t *p, uint8_t inv) {
    ssd1306_write(p, SET_NORM_INV | (inv & 1));
}

inline void ssd1306_flip(ssd1306_t *p, uint8_t flip) {
    if (flip) {
        ssd1306_write(p, SET_SEG_REMAP | 0x00);
        ssd1306_write(p, SET_COM_OUT_DIR | 0x00);
    } else {
        ssd1306_write(p, SET_SEG_REMAP | 0x01);
        ssd1306_write(p, SET_COM_OUT_DIR | 0x08);
    }
}

inline void ssd1306_clear(ssd1306_t *p, uint8_t colour) {
    memset(p->buffer, 0xff*(colour > 0), p->bufsize);
}

inline void ssd1306_type(ssd1306_t *p, uint8_t type) {
    if (type) {
        p->disp_col_offset = 2;
    } else {
        p->disp_col_offset = 0;
    }
}


void ssd1306_draw_pixel(ssd1306_t *p, int32_t x, int32_t y, uint8_t colour) {
    if(x>=p->width || y>=p->height) return;
    if(x<0 || y<0) return;

    // each page is 129 bytes long for the oled cmd byte at the start
    if(colour == 2) {
        p->buffer[1 + x + (1 + p->width)*(y>>3) ] ^= 0x1<<(y&0x07);
    } else if (colour) {
        p->buffer[1 + x + (1 + p->width)*(y>>3) ] |= 0x1<<(y&0x07); // y>>3==y/8 && y&0x7==y%8
    } else {
        p->buffer[1 + x + (1 + p->width)*(y>>3) ] &= ~(0x1<<(y&0x07)); // y>>3==y/8 && y&0x7==y%8
    }
}

void ssd1306_draw_line(ssd1306_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t colour) {
    if(x1>x2) {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    if(x1==x2) {
        if(y1>y2)
            swap(&y1, &y2);
        for(int32_t i=y1; i<=y2; ++i)
            ssd1306_draw_pixel(p, x1, i, colour);
        return;
    }

    float m=(float) (y2-y1) / (float) (x2-x1);

    for(int32_t i=x1; i<=x2; ++i) {
        float y=m*(float) (i-x1)+(float) y1;
        ssd1306_draw_pixel(p, i, (int32_t) y, colour);
    }
}

void ssd1306_fill_rectangle(ssd1306_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour) {
    for(int32_t i=0; i<width; ++i)
        for(int32_t j=0; j<height; ++j)
            ssd1306_draw_pixel(p, x+i, y+j, colour);

}

void ssd1306_draw_rectangle(ssd1306_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour) {
    ssd1306_draw_line(p, x, y, x+width, y, colour);
    ssd1306_draw_line(p, x, y+height, x+width, y+height, colour);
    ssd1306_draw_line(p, x, y, x, y+height, colour);
    ssd1306_draw_line(p, x+width, y, x+width, y+height, colour);
}

void ssd1306_draw_char_with_font(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, char c, uint8_t colour) {
    if(c<font[3]||c>font[4])
        return;

if (colour == 1)
    ssd1306_fill_rectangle( p, x, y, (font[1]+font[2])*scale, font[0]*scale, 0);
if (colour == 0)
    ssd1306_fill_rectangle( p, x, y, (font[1]+font[2])*scale, font[0]*scale, 1);
if (colour == 2) {} // do nothing to allow OR/XORing

    uint32_t parts_per_line=(font[0]>>3)+((font[0]&7)>0);
    for(uint8_t w=0; w<font[1]; ++w) { // width
        uint32_t pp=(c-font[3])*font[1]*parts_per_line+w*parts_per_line+5;
        for(uint32_t lp=0; lp<parts_per_line; ++lp) {
            uint8_t line=font[pp];

            for(int8_t j=0; j<8; ++j, line>>=1) {
                if(line & 1)
                    ssd1306_fill_rectangle(p, x+w*scale, y+((lp<<3)+j)*scale, scale, scale, colour);
            }

            ++pp;
        }
    }
}

void ssd1306_draw_string_with_font(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, const char *s, uint8_t colour) {
    for(int32_t x_n=x; *s; x_n+=(font[1]+font[2])*scale) {
        ssd1306_draw_char_with_font(p, x_n, y, scale, font, *(s++), colour);
    }
}

void ssd1306_draw_char(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, char c, uint8_t colour) {
    ssd1306_draw_char_with_font(p, x, y, scale, font_8x5, c, colour);
}

void ssd1306_draw_string(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const char *s, uint8_t colour) {
    ssd1306_draw_string_with_font(p, x, y, scale, font_8x5, s, colour);
}

static inline uint32_t ssd1306_bmp_get_val(const uint8_t *data, const size_t offset, uint8_t size) {
    switch(size) {
    case 1:
        return data[offset];
    case 2:
        return data[offset]|(data[offset+1]<<8);
    case 4:
        return data[offset]|(data[offset+1]<<8)|(data[offset+2]<<16)|(data[offset+3]<<24);
    default:
        __builtin_unreachable();
    }
    __builtin_unreachable();
}

void ssd1306_bmp_show_image_with_offset(ssd1306_t *p, const uint8_t *data, const long size, uint32_t x_offset, uint32_t y_offset) {
    if(size<54) // data smaller than header
        return;

    const uint32_t bfOffBits=ssd1306_bmp_get_val(data, 10, 4);
    const uint32_t biSize=ssd1306_bmp_get_val(data, 14, 4);
    const int32_t biWidth=(int32_t) ssd1306_bmp_get_val(data, 18, 4);
    const int32_t biHeight=(int32_t) ssd1306_bmp_get_val(data, 22, 4);
    const uint16_t biBitCount=(uint16_t) ssd1306_bmp_get_val(data, 28, 2);
    const uint32_t biCompression=ssd1306_bmp_get_val(data, 30, 4);

    if(biBitCount!=1) // image not monochrome
        return;

    if(biCompression!=0) // image compressed
        return;

    const int table_start=14+biSize;
    uint8_t color_val = 0;

    for(uint8_t i=0; i<2; ++i) {
        if(!((data[table_start+i*4]<<16)|(data[table_start+i*4+1]<<8)|data[table_start+i*4+2])) {
            color_val=i;
            break;
        }
    }

    uint32_t bytes_per_line=(biWidth/8)+(biWidth&7?1:0);
    if(bytes_per_line&3)
        bytes_per_line=(bytes_per_line^(bytes_per_line&3))+4;

    const uint8_t *img_data=data+bfOffBits;

    int step=biHeight>0?-1:1;
    int border=biHeight>0?-1:biHeight;
    for(uint32_t y=biHeight>0?biHeight-1:0; y!=border; y+=step) {
        for(uint32_t x=0; x<biWidth; ++x) {
            if(((img_data[x>>3]>>(7-(x&7)))&1)==color_val)
                ssd1306_draw_pixel(p, x_offset+x, y_offset+y, 1);
        }
        img_data+=bytes_per_line;
    }
}

inline void ssd1306_bmp_show_image(ssd1306_t *p, const uint8_t *data, const long size) {
    ssd1306_bmp_show_image_with_offset(p, data, size, 0, 0);
}

static void _draw_page(ssd1306_t *p)
{
    uint8_t payload[] = {(0x00 | (p->disp_col_offset & 0x0F)), (0x10 | (p->disp_col_offset >> 4)), (0xB0 | (p->curr_page & 0x0F))};
    uint8_t *buf = &p->mem[p->bufsize * (p->curr_buf ^ 1)];

    for (size_t i = 0; i < sizeof(payload); ++i)
        ssd1306_write(p, payload[i]);

    buf[p->curr_page * (1 + p->width)] = 0x40; // oled data not cmd
    fancy_write(p->i2c_i, p->address, &buf[p->curr_page * (1 + p->width)], p->width + 1, "ssd1306_show");
}

void ssd1306_show(ssd1306_t *p)
{
    if (p->curr_page == 0)
    {
        p->curr_buf ^= 1;
        p->buffer = &p->mem[p->bufsize * p->curr_buf];
        memcpy(p->buffer, &p->mem[p->bufsize * (p->curr_buf ^ 1)], p->bufsize); // needed for scrolling
        _draw_page(p);
        p->curr_page++;
    } else {
        p->show_pending = true;
    }
}

bool ssd1306_show_continue(ssd1306_t *p)
{
    bool finished = true;

    if (p->curr_page > 0)
    {
        _draw_page(p);
        p->curr_page++;
        if (p->curr_page == p->pages)
        {
            p->curr_page = 0;
            if(p->show_pending)
            {
                p->show_pending = false;
                ssd1306_show(p);
            }
        }
        else
        {
            finished = false;
        }
    }

    return finished;
}