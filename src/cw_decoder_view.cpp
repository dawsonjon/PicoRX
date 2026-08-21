#include "cw_decoder_view.h"
#include <cstdio>
#include <cstring>
#include "header.h"
#include "font_8x5.h"

#include "pico/stdlib.h"


//COLOUR SCHEME
const uint16_t BACKGROUND        = __builtin_bswap16(0x0862);
const uint16_t PANEL_BG          = COLOUR_NAVY;
const uint16_t DIVIDER_LINE      = __builtin_bswap16(0x05bf);
const uint16_t INACTIVE_BORDER   = __builtin_bswap16(0x218a);
const uint16_t ACTIVE_BORDER     = COLOUR_AQUA;
const uint16_t TEXT_HEADING      = COLOUR_AQUA;
const uint16_t TEXT_PRIMARY      = COLOUR_WHITE;
const uint16_t TEXT_RECENT       = COLOUR_AQUA;
const uint16_t TEXT_DIM          = COLOUR_GREY;
const uint16_t TEXT_PARTIAL      = COLOUR_ORANGE;
const uint16_t TX_BORDER         = COLOUR_ORANGE;
const uint16_t TX_BG             = COLOUR_NAVY;
const uint16_t TX_TEXT           = COLOUR_ORANGE;

//LAYOUT
const uint16_t PADDING = 2;
const uint16_t HEADING_Y = 0;
const uint16_t HEADING_HEIGHT = 24;
const uint16_t PANEL_X = 4;
const uint16_t PANEL_Y = HEADING_Y + HEADING_HEIGHT + 4;
const uint16_t PANEL_WIDTH = 320 - 8;
const uint16_t WATERFALL_HEIGHT = 5;
const uint16_t WATERFALL_WIDTH = PANEL_WIDTH - (2*PADDING);
const uint16_t TEXT_HEIGHT = 8;
const uint16_t PANEL_HEIGHT = PADDING + TEXT_HEIGHT + PADDING + TEXT_HEIGHT + PADDING + WATERFALL_HEIGHT + PADDING;
const uint16_t PANEL_INTERVAL = PANEL_HEIGHT + PADDING;
const uint16_t TX_PANEL_Y = PANEL_Y+PANEL_INTERVAL*NUM_CHANNELS + 4;
const uint16_t TX_PANEL_HEIGHT = PADDING + TEXT_HEIGHT + PADDING + TEXT_HEIGHT;

void draw_cw_decoder(ILI934X *display) {
  display->clear(BACKGROUND);
  for(int channel=0; channel<NUM_CHANNELS; ++channel)
  {
    uint16_t Y = PANEL_Y + (PANEL_INTERVAL * channel);
    display->fillRect(PANEL_X, Y, PANEL_HEIGHT, PANEL_WIDTH, PANEL_BG);
  }
  display->writeImage(0, 0, 320, HEADING_HEIGHT, header);

  for(int channel=0; channel<NUM_CHANNELS; ++channel)
  {
    uint16_t TEXT_Y = PANEL_Y + (PANEL_INTERVAL * channel) + PADDING;
    char buffer[50];
    snprintf(buffer, 50, "chan: %u %9.3fkHz %3u%% %3.0fwpm %3.0fdB(500 Hz)", channel, 0.0f, 0, 0.0f, -30.0f);

    int TEXT_X = (320 - (strlen(buffer) * 6))/2;
    display->drawString(TEXT_X, TEXT_Y, font_8x5, buffer, TEXT_HEADING, PANEL_BG);
  }
}

void draw_waterfall(c_cw_dsp &cw_dsp, ILI934X *display)
{
  static uint16_t waterfall_newest = 0;
  static uint16_t waterfall[FRAME_SIZE/2][WATERFALL_WIDTH] = {0};

  uint32_t *magnitudes = cw_dsp.get_magnitudes();

  for(uint16_t bin=0; bin<FRAME_SIZE/2; ++bin) {
    uint32_t magnitude = magnitudes[bin];
    float pixel = std::max(0.0, 56*log10(magnitude+1));
    waterfall[bin][waterfall_newest] = display->colour565(0, pixel, pixel/2);
  }

  static uint32_t refresh_count = 0;
  if(++refresh_count == 4) {
    refresh_count = 0;
    uint8_t channel = 0;
    uint8_t sub_y = 0;
    for(uint16_t cw_waterfall_y=0; cw_waterfall_y<30; ++cw_waterfall_y) {
      uint8_t y = PANEL_Y + (channel * PANEL_INTERVAL) + PADDING+TEXT_HEIGHT+PADDING+TEXT_HEIGHT+PADDING + sub_y;
      display->writeHLine(PANEL_X + PADDING, y, WATERFALL_WIDTH - waterfall_newest - 1, &waterfall[cw_waterfall_y][waterfall_newest + 1]);
      display->dmaFlush();
      display->writeHLine(PANEL_X + PADDING + WATERFALL_WIDTH - waterfall_newest, y, waterfall_newest+1, waterfall[cw_waterfall_y]);
      display->dmaFlush();
      if(++sub_y > 4) {
        sub_y = 0;
        channel++;
      }
    }
  }

  if(++waterfall_newest == WATERFALL_WIDTH) waterfall_newest = 0;
}

void draw_channel_status(my_cw_dsp &cw_dsp, ILI934X *display, uint32_t base_frequency)
{
  for(int channel=0; channel<NUM_CHANNELS; ++channel)
  {
    uint16_t TEXT_Y = PANEL_Y + (PANEL_INTERVAL * channel) + PADDING;
    char buffer[50];
    float frequency = base_frequency+((2 * channel * 586)+586)/2;

    int TEXT_X = (320 - (44 * 6))/2;
    snprintf(buffer, 50, "%9.3f", frequency/1000);
    display->drawString(TEXT_X+8*6, TEXT_Y, font_8x5, buffer, TEXT_HEADING, PANEL_BG);
    snprintf(buffer, 50, "%3lu%%", cw_dsp.get_buffer_percent(channel));
    display->drawString(TEXT_X+21*6, TEXT_Y, font_8x5, buffer, TEXT_HEADING, PANEL_BG);
    snprintf(buffer, 50, "%3.0f", cw_dsp.get_WPM(channel));
    display->drawString(TEXT_X+25*6, TEXT_Y, font_8x5, buffer, TEXT_HEADING, PANEL_BG);
    snprintf(buffer, 50, "%3.0f", cw_dsp.get_snr(channel));
    display->drawString(TEXT_X+32*6, TEXT_Y, font_8x5, buffer, TEXT_HEADING, PANEL_BG);
  }
}

void my_cw_dsp :: decode(uint16_t cluster, std::string text, std::string partial)
{

  const int decoded_text_size = decoded_text[cluster].size();
  const int new_text_size = text.size();
  const int partial_size = partial.size();

  const int font_width = 6;
  const int text_width = (PANEL_WIDTH - (2*PADDING))/font_width;
  int excess_length = decoded_text_size + new_text_size + partial_size - text_width;
  if(excess_length > 0) {
    int chars_to_remove = std::min(excess_length, decoded_text_size);
    decoded_text[cluster].erase(0, chars_to_remove);
    excess_length -= chars_to_remove;
  }
  if(excess_length > 0) {
    int chars_to_remove = std::min(excess_length, new_text_size);
    text.erase(0, chars_to_remove);
    excess_length -= chars_to_remove;
  }

  const uint16_t TEXT_Y = PANEL_Y + (PANEL_INTERVAL * cluster) + PADDING + TEXT_HEIGHT + PADDING;
  const uint16_t TEXT_X = PANEL_X + PADDING;
  display->drawString(TEXT_X, TEXT_Y, font_8x5, decoded_text[cluster].c_str(), TEXT_PRIMARY, PANEL_BG);
  display->drawString(TEXT_X+(font_width*decoded_text[cluster].size()),                 TEXT_Y, font_8x5, text.c_str(), TEXT_RECENT, PANEL_BG);
  display->drawString(TEXT_X+(font_width*(decoded_text[cluster].size() + text.size())), TEXT_Y, font_8x5, partial.c_str(), TEXT_PARTIAL, PANEL_BG);

  decoded_text[cluster]+=text;
}
