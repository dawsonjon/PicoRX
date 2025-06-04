#include "waterfall.h"

#include <cmath>
#include <cstdio>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "fft_filter.h"
#include "ili934x.h"
#include "font_seven_segment_25x15.h"
#include "font_16x12.h"
#include "font_8x5.h"
#include "rx_definitions.h"
#include "ui.h"
#include "pins.h"
#define SPI_PORT spi1
#include "codecs/decode_sstv.h"

waterfall::waterfall()
{
    //using ili9341 library from here:
    //https://github.com/bizzehdee/pico-libs
    spi_init(SPI_PORT, 75000000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, 320, 240);
}

waterfall::~waterfall()
{
    delete display;
}

void waterfall::configure_display(uint8_t settings, bool invert_colours, bool invert_display, uint8_t display_driver)
{

    e_display_type display_type = display_driver?ILI9341:ILI9341_2;
    if(settings == 0)
    {
      enabled = false;
    } 
    else if(settings == 1)
    {
      enabled = true;
      display->init(R0DEG, invert_colours, invert_display, display_type);
    }
    else if(settings == 2)
    {
      enabled = true;
      display->init(R180DEG, invert_colours, invert_display, display_type);
    }
    else if(settings == 3)
    {
      enabled = true;
      display->init(MIRRORED0DEG, invert_colours, invert_display, display_type);
    }
    else if(settings == 4)
    {
      enabled = true;
      display->init(MIRRORED180DEG, invert_colours, invert_display, display_type);
    }
    else if(settings == 5)
    {
      enabled = true;
      display->init(R90DEG, invert_colours, invert_display, display_type);
    }
    else if(settings == 6)
    {
      enabled = true;
      display->init(R270DEG, invert_colours, invert_display, display_type);
    }
    else if(settings == 7)
    {
      enabled = true;
      display->init(MIRRORED90DEG, invert_colours, invert_display, display_type);
    }
    else if(settings == 8)
    {
      enabled = true;
      display->init(MIRRORED270DEG, invert_colours, invert_display, display_type);
    }

    if(enabled)
    {
       refresh = true;
       display->powerOn(true);
       draw();
    }
    else
    {
       display->powerOn(false);
    }
}

void waterfall::powerOn(bool state)
{
    power_state = state;
    if(enabled && state)
    {
       refresh = true;
       display->powerOn(true);
       draw();
    }
    else
    {
       display->powerOn(false);
    }
}

const uint16_t smeter_height = 165;
const uint16_t smeter_bar_width = 4;
const uint16_t smeter_x = 266;
const uint16_t smeter_y = 65;
const uint16_t waterfall_height = 80u;
const uint16_t waterfall_x = 4u;
const uint16_t waterfall_y = 157u;
const uint16_t num_cols = 256u;
const uint16_t scope_height = 80u;
const uint16_t scope_x = 4u;
const uint16_t scope_y = 61u;
const uint16_t dial_width = 320u;

void waterfall::draw()
{

    if(m_aux_display_state == sstv_active)
    {
      const char text[] = "SSTV Decoder";
      uint16_t text_width = strlen(text) * 12;
      uint16_t text_height = 16;
      uint16_t box_width = text_width + 20;
      uint16_t box_height = text_height + 20;
      display->clear(COLOUR_BLUE);
      display->fillRoundedRect((320-box_width)/2, (240-box_height)/2, box_height, box_width, 5, COLOUR_BLACK);
      display->drawString((320-text_width)/2, (240-text_height)/2, font_16x12, "SSTV Decoder", COLOUR_WHITE, COLOUR_BLACK);

      return;
    }

    display->clear();

    //draw borders
    //Horizontal
    display->drawFastHline(3, num_cols+3, waterfall_y-1,                  display->colour565(255,255,255));
    display->drawFastHline(3, num_cols+3, waterfall_y+waterfall_height+1, display->colour565(255,255,255));
    display->drawFastHline(3, num_cols+3, scope_y+scope_height+1,         display->colour565(255,255,255));
    display->drawFastHline(3, num_cols+3, scope_y-1,                      display->colour565(255,255,255));

    //Vertical
    display->drawFastVline(3,          scope_y-1,     scope_y+scope_height+1,         display->colour565(255,255,255));
    display->drawFastVline(num_cols+4, scope_y-1,     scope_y+scope_height+1,         display->colour565(255,255,255));
    display->drawFastVline(3,          waterfall_y-1, waterfall_y+waterfall_height+1, display->colour565(255,255,255));
    display->drawFastVline(num_cols+4, waterfall_y-1, waterfall_y+waterfall_height+1, display->colour565(255,255,255));

    //smeter outline
    display->drawFastHline(smeter_x+13, smeter_x+20, smeter_y-2,               display->colour565(255,255,255));
    display->drawFastHline(smeter_x+13, smeter_x+20, smeter_y+smeter_height+3, display->colour565(255,255,255));
    display->drawFastVline(smeter_x+13, smeter_y-1,  smeter_y+smeter_height+3, display->colour565(255,255,255));
    display->drawFastVline(smeter_x+20, smeter_y-1,  smeter_y+smeter_height+3, display->colour565(255,255,255));


    //draw smeter
    const char smeter[13][4]  = {"s0 ", "s1 ", "s2 ", "s3 ", "s4 ", "s5 ", "s6 ", "s7 ", "s8 ", "s9 ", "+10", "+20", "+30"};
    for(int16_t s=0; s<13; s++)
    {
      uint16_t s_px = dBm_to_px(S_to_dBm(s), smeter_height);
      uint16_t y = smeter_y+1 + smeter_height - s_px;
      display->drawLine(smeter_x+20, y, smeter_x+22,  y, display->colour565(255,255,255));
      uint16_t colour;
      switch(s)
      {
        case 12: colour = COLOUR_RED; break;
        case 11: colour = COLOUR_YELLOW; break;
        case 10: colour = COLOUR_GREEN; break;
        default: colour = COLOUR_WHITE; break;
      }
      display->drawString(smeter_x+25,  y-4, font_8x5, smeter[s], colour, COLOUR_BLACK);
    }
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
      r = (uint16_t)r-(r>>2) + (blend_r>>2);
      g = (uint16_t)g-(g>>2) + (blend_g>>2);
      b = (uint16_t)b-(b>>2) + (blend_b>>2);
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

void waterfall::update_spectrum(rx &receiver, s_settings &ui_settings, rx_settings &settings, rx_status &status, uint8_t spectrum[], uint8_t dB10, uint8_t zoom)
{
    if(!enabled) return;
    if(!power_state) return;

    //state machine to select other display options
    switch(m_aux_display_state)
    {
      case waterfall_active:
        if(ui_settings.global.aux_view == 1)
        {
          m_aux_display_state = sstv_active;
          draw();
        }
        break;

      case sstv_active:
        if(ui_settings.global.aux_view == 0)
        {
          m_aux_display_state = waterfall_active;
          draw();
          refresh = true;
        }
        break;
    }
    if(m_aux_display_state == sstv_active)
    {
      decode_sstv(receiver);
      return;
    }
    
    //update spectrum and waterfall display
    const uint16_t scope_fg = display->colour565(255, 255, 255);
    static uint16_t top_row = 0u;

    //scroll waterfall
    top_row = top_row?top_row-1:waterfall_height-1;

    //add new line to waterfall
    for(uint16_t col=0; col<num_cols; col++)
    {
      waterfall_buffer[top_row][col] = spectrum[col];
    }

    receiver.access(false);
    const int16_t power_dBm = status.signal_strength_dBm;
    receiver.release();

    static float filtered_power = power_dBm;
    filtered_power = (filtered_power * 0.7) + (power_dBm * 0.3);

    static float last_filtered_power = 0;
    static uint8_t last_squelch = 255;
    if(abs(filtered_power - last_filtered_power) > 1 || settings.squelch_threshold != last_squelch|| refresh)
    {
      last_filtered_power = filtered_power;
      last_squelch = settings.squelch_threshold;
      uint16_t power_px = dBm_to_px(filtered_power, smeter_height+6);
      uint16_t squelch_px = dBm_to_px(S_to_dBm(settings.squelch_threshold), smeter_height);

      uint16_t colour=heatmap(dBm_to_px(filtered_power, 255));
      display->fillRect(smeter_x+15, smeter_y+4+smeter_height-power_px, power_px, smeter_bar_width, colour);
      display->fillRect(smeter_x+15, smeter_y+1,    smeter_height-power_px, smeter_bar_width, COLOUR_BLACK);
      display->fillRect(smeter_x+15, smeter_y+0+smeter_height-squelch_px, 3, smeter_bar_width, COLOUR_RED);
      display->fillRect(smeter_x+15, smeter_y+1+smeter_height-squelch_px, 1, smeter_bar_width, COLOUR_WHITE);

      char buffer[9];
      snprintf(buffer, 9, "%4.0fdBm", filtered_power);
      display->drawString(233, 31, font_16x12, buffer, COLOUR_GREEN, COLOUR_BLACK);

    }

    //draw status
    static int16_t last_mode = -1;
    if(settings.mode != last_mode || refresh)
    {
      last_mode = settings.mode;
      const char modes[6][4]  = {"AM ", "AMS", "LSB", "USB", "FM ", "CW "};
      display->drawString(168, 31, font_16x12, modes[settings.mode], COLOUR_YELLOW, COLOUR_BLACK);
    }

    static uint8_t last_zoom = 255;
    if(zoom != last_zoom || refresh)
    {
      last_zoom = zoom;
      display->fillRect(waterfall_x,  waterfall_y-12, 8, 256, COLOUR_BLACK);

      const int32_t kHz_per_tick = zoom>=3?1:5;
      const int32_t bins_per_tick = (256*kHz_per_tick*zoom)/30;

      uint16_t freq_kHz = 0;
      for(uint16_t bin = 0; bin < 110; bin += bins_per_tick)
      {
        char buffer[7];
        sprintf(buffer, "%i", freq_kHz);
        uint16_t x = waterfall_x + num_cols/2 + bin - ((strlen(buffer)-1)*3);
        display->drawString(x,  waterfall_y-11, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);

        sprintf(buffer, "%i", -freq_kHz);
        x = waterfall_x + num_cols/2 - bin - ((strlen(buffer)-1)*6);
        display->drawString(x,  waterfall_y-11, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);
        freq_kHz += kHz_per_tick;
      }
    }

    //draw dial
    const uint8_t pixels_per_kHz = 5;
    const uint16_t scale_y = 1;

    static uint32_t last_frequency =0;
    if(settings.tuned_frequency_Hz != last_frequency || refresh)
    { 
      last_frequency = settings.tuned_frequency_Hz;

      display->fillRect(0, scale_y, 25, dial_width, COLOUR_BLACK);
      display->fillRect(0, scale_y+25, 1, dial_width, COLOUR_WHITE);
      display->fillRect(0, scale_y, 1, dial_width, COLOUR_WHITE);
      for(uint16_t x = 0; x<dial_width; x++)
      {
        uint32_t rounded_frequency_Hz = round(settings.tuned_frequency_Hz / 1000)*1000;
        uint32_t pixel_frequency_Hz = rounded_frequency_Hz + (((x-(dial_width/2))*1000)/pixels_per_kHz);
        //draw 1kHz tick Marks
        if(pixel_frequency_Hz%1000 == 0)
        {
          display->fillRect(x, scale_y+20, 5, 1, COLOUR_WHITE);
        }
        //draw 5kHz tick Marks
        if(pixel_frequency_Hz%5000 == 0)
        {
          display->fillRect(x, scale_y+17, 5, 1, COLOUR_WHITE);
        }
        //draw 10kHz tick Marks
        if(pixel_frequency_Hz%10000 == 0)
        {
          if(x > 20 && (x + 20) < dial_width)
          {
            char buffer[10];
            snprintf(buffer, 10, "%2lu.%02lu", pixel_frequency_Hz/1000000, ((pixel_frequency_Hz%1000000)/10000));
            display->drawString(x-18,  scale_y+5, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);
          }
          display->fillRect(x, scale_y+15, 10, 1, COLOUR_WHITE);
          display->fillRect(x, scale_y, 2, 1, COLOUR_WHITE);
        }
      }
      display->fillRect(dial_width/2-1, scale_y, 25, 3, COLOUR_RED);
      display->fillRect(dial_width/2, scale_y, 25, 1, COLOUR_WHITE);


    }

    //draw scope
    uint8_t data_points[num_cols];
    for(uint16_t scope_col=0; scope_col<num_cols; ++scope_col)
    {
      data_points[scope_col] = (scope_height * (uint16_t)waterfall_buffer[top_row][scope_col])/270;
    }
    uint16_t tick_spacing;
    if(zoom >= 3)
    {
      tick_spacing = 256*zoom/30; //place ticks at 1kHz steps
    }
    else
    {
      tick_spacing = 256*zoom*5/30; //place ticks at 5kHz steps
    }
    for(uint16_t scope_row = 0; scope_row < scope_height; ++scope_row)
    {
       uint16_t hline[num_cols];
       uint16_t waterfall_line[num_cols];
       uint16_t row_address = (top_row+scope_row)%waterfall_height;

       uint16_t scope_row_colour = heatmap((uint16_t)scope_row*256/scope_height, false, false);
       uint16_t scope_row_colour_passband = heatmap((uint16_t)scope_row*256/scope_height, true, false);
       uint16_t scope_row_colour_cursor = heatmap((uint16_t)scope_row*256/scope_height, true, true);
       const bool row_is_tick = (scope_row%(scope_height*dB10/270)) == 0;

       //draw one line of scope
       for(uint16_t scope_col=0; scope_col<num_cols; ++scope_col)
       {
 
         uint8_t data_point = data_points[scope_col];//(scope_height * (uint16_t)waterfall_buffer[top_row][scope_col])/270;

         const int16_t fbin = scope_col-128;
         const bool is_usb_col = (fbin > (status.filter_config.start_bin * zoom)) && (fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.upper_sideband;
         const bool is_lsb_col = (-fbin > (status.filter_config.start_bin * zoom)) && (-fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.lower_sideband;
         const bool is_passband = is_usb_col || is_lsb_col;
         const bool col_is_tick = (fbin%tick_spacing == 0) && fbin;
 
         if(scope_row < data_point)
         {
           uint16_t colour = scope_row_colour;
           colour = is_passband?scope_row_colour_passband:colour;
           colour = fbin==0?scope_row_colour_cursor:colour;
           hline[scope_col] = colour;
         }
         else if(scope_row == data_point)
         {
           hline[scope_col] = scope_fg;
         }
         else
         {
           uint16_t colour = heatmap(0, is_passband, fbin==0);
           colour = col_is_tick?COLOUR_GREY:colour;
           colour = row_is_tick?COLOUR_GREY:colour;
           hline[scope_col] = colour;
         }
       }
       display->dmaFlush();
       display->writeHLine(scope_x, scope_y+scope_height-1-scope_row, num_cols, hline);

       //draw 1 line of waterfall
       for(uint16_t scope_col=0; scope_col<num_cols; ++scope_col)
       {
         const int16_t fbin = scope_col-128;
         const bool is_usb_col = (fbin > (status.filter_config.start_bin * zoom)) && (fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.upper_sideband;
         const bool is_lsb_col = (-fbin > (status.filter_config.start_bin * zoom)) && (-fbin < (status.filter_config.stop_bin * zoom)) && status.filter_config.lower_sideband;
         const bool is_passband = is_usb_col || is_lsb_col;
 
         uint8_t heat = waterfall_buffer[row_address][scope_col];
         uint16_t colour=heatmap(heat, is_passband, fbin==0);
         waterfall_line[scope_col] = colour;
       }
       display->dmaFlush();
       display->writeHLine(waterfall_x, scope_row+waterfall_y, num_cols, waterfall_line);

     }
     display->dmaFlush();


    //draw frequency
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
    if(lastMHz!=MHz || lastkHz!=kHz || lastHz!=Hz || refresh)
    {
      char buffer[20];
      snprintf(buffer, 20, "%02lu:%03lu:%03lu", MHz, kHz, Hz);
      display->drawString(3, 31, font_seven_segment_25x15, buffer, COLOUR_RED, COLOUR_BLACK);
      lastMHz = MHz;
      lastkHz = kHz;
      lastHz = Hz;
    }

    refresh = false;

}


  //sstv_decoder.set_auto_slant_correction(ENABLE_SLANT_CORRECTION);
  //sstv_decoder.set_timeout_seconds(LOST_SIGNAL_TIMEOUT_SECONDS);

#define STRETCH true
void waterfall::decode_sstv(rx &receiver)
{
  int16_t i, q;
  static uint16_t last_pixel_y=0;
  static uint8_t line_rgb[320][4];
  static c_sstv_decoder sstv_decoder(15000);
  static s_sstv_mode *modes = sstv_decoder.get_modes();

  uint16_t samples_processed = 0;

  #ifdef MONITOR_BUFFER_LEVEL  

  //Usually gets serviced about once every 50ms.  This can take longer if the
  //UI is busy.  The length of the queue needs to be adjusted to the the queue
  //never fills up while the CPU is busy doing other things.

  //It probably isn't the end of the world if we run out of space while tuning
  //around the bands or doing stuff, its unlikely we could decode anything
  //anyway if this was the case, but we don't want it to fill up when we aren't
  //doing anything.

  static uint32_t start_time = 0;
  uint32_t duration = time_us_32() - start_time;
  printf("buffer level: %lu time: %lu\n", receiver.get_iq_buffer_level(), duration);
  start_time = time_us_32();
  #endif

  while(receiver.get_raw_data(i, q))
  {

      samples_processed++;
      uint16_t pixel_y;
      uint16_t pixel_x;
      uint8_t pixel_colour;
      uint8_t pixel;
      int16_t frequency;
      const bool new_pixel = sstv_decoder.decode_iq(i, q, pixel_y, pixel_x, pixel_colour, pixel, frequency);

      if(new_pixel)
      {
          e_sstv_mode mode = sstv_decoder.get_mode();

          if(pixel_y > last_pixel_y)
          {

            //convert from 24 bit to 16 bit colour
            uint16_t line_rgb565[320];
            uint16_t scaled_pixel_y = 0;

            if(mode == pd_50 || mode == pd_90 || mode == pd_120 || mode == pd_180)
            {

              //rescale imaagesto fit on screen
              if(mode == pd_120 || mode == pd_180)
              {
                scaled_pixel_y = (uint32_t)last_pixel_y * 240 / 496; 
              }
              else
              {
                scaled_pixel_y = last_pixel_y;
              }

              for(uint16_t x=0; x<320; ++x)
              {
                int16_t y  = line_rgb[x][0];
                int16_t cr = line_rgb[x][1];
                int16_t cb = line_rgb[x][2];
                cr = cr - 128;
                cb = cb - 128;
                int16_t r = y + 45 * cr / 32;
                int16_t g = y - (11 * cb + 23 * cr) / 32;
                int16_t b = y + 113 * cb / 64;
                r = r<0?0:(r>255?255:r);
                g = g<0?0:(g>255?255:g);
                b = b<0?0:(b>255?255:b);
                line_rgb565[x] = display->colour565(r, g, b);
              }
              display->writeHLine(0, scaled_pixel_y*2, 320, line_rgb565);
              display->dmaFlush();
              for(uint16_t x=0; x<320; ++x)
              {
                int16_t y  = line_rgb[x][3];
                int16_t cr = line_rgb[x][1];
                int16_t cb = line_rgb[x][2];
                cr = cr - 128;
                cb = cb - 128;
                int16_t r = y + 45 * cr / 32;
                int16_t g = y - (11 * cb + 23 * cr) / 32;
                int16_t b = y + 113 * cb / 64;
                r = r<0?0:(r>255?255:r);
                g = g<0?0:(g>255?255:g);
                b = b<0?0:(b>255?255:b);
                line_rgb565[x] = display->colour565(r, g, b);
              }
              display->writeHLine(0, scaled_pixel_y*2 + 1, 320, line_rgb565);
              display->dmaFlush();
            }
            else
            {
              for(uint16_t x=0; x<320; ++x)
              {
                line_rgb565[x] = display->colour565(line_rgb[x][0], line_rgb[x][1], line_rgb[x][2]);
              }
              display->writeHLine(0, last_pixel_y, 320, line_rgb565);
              display->dmaFlush();
              
            }
            for(uint16_t x=0; x<320; ++x) line_rgb[x][0] = line_rgb[x][1] = line_rgb[x][2] = 0;

            //update progress
            display->fillRect(320-(21*6)-2, 240-10, 10, 21*6+2, COLOUR_BLACK);
            char buffer[30];
            if(mode==martin_m1)
            {
              snprintf(buffer, 30, "Martin M1: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==martin_m2)
            {
              snprintf(buffer, 30, "Martin M2: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==scottie_s1)
            {
              snprintf(buffer, 30, "Scottie S1: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==scottie_s2)
            {
              snprintf(buffer, 30, "Scottie S2: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==sc2_120)
            {
              snprintf(buffer, 30, "SC2 120: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==pd_50)
            {
              snprintf(buffer, 30, "PD 50: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==pd_90)
            {
              snprintf(buffer, 30, "PD 90: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==pd_120)
            {
              snprintf(buffer, 30, "PD 120: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            else if(mode==pd_180)
            {
              snprintf(buffer, 30, "PD 180: %ux%u", modes[mode].width, last_pixel_y+1);
            }
            display->drawString(320-(21*6), 240-8, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLACK);

          }
          last_pixel_y = pixel_y;
          

          if(pixel_x < 320 && pixel_y < 256 && pixel_colour < 4) {
            if(STRETCH && modes[mode].width==160)
            {
              if(pixel_x < 160)
              {
                line_rgb[pixel_x*2][pixel_colour] = pixel;
                line_rgb[pixel_x*2+1][pixel_colour] = pixel;
              }
            }
            else
            {
              line_rgb[pixel_x][pixel_colour] = pixel;
            }

          }
          
      }
   }
}
