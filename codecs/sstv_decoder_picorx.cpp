#include <algorithm>

#include "../src/font_16x12.h"
#include "../src/font_8x5.h"
#include "../src/ili934x.h"
#include "sstv_decoder_picorx.h"

void c_sstv_decoder_picorx ::image_write_line(uint16_t line_rgb565[], uint16_t y, uint16_t width,
                                              uint16_t height, const char* mode_string)
{

  // scale image to fit TFT size
  uint16_t scaled_row[display_width];
  uint16_t pixel_number = 0;
  for (uint16_t x = 0; x < width; x++) {
    uint16_t scaled_x = static_cast<uint32_t>(x) * display_width / width;
    while (pixel_number < scaled_x) {
      // display expects byteswapped data
      scaled_row[pixel_number] = ((line_rgb565[x] & 0xff) << 8) | ((line_rgb565[x] & 0xff00) >> 8);
      pixel_number++;
    }
  }

  uint32_t scaled_y = static_cast<uint32_t>(y) * display_height / height;
  while (row_number < scaled_y) {
    display->writeHLine(0, row_number, display_width, scaled_row);
    display->dmaFlush();
    row_number++;
  }

  // update progress
  char buffer[27];
  snprintf(buffer, 27, "%10s: %ux%u  ", mode_string, width, y + 1);
  display->drawString(0, display_height + 10, font_8x5, buffer, COLOUR_WHITE, COLOUR_BLUE);
}

void c_sstv_decoder_picorx ::scope(uint16_t mag, int16_t freq)
{

  static const uint16_t scope_x = 168;
  static const uint16_t scope_y = 238;
  static const uint16_t scope_width = 150;

  static uint8_t row = 0;
  static uint16_t count = 0;
  static uint32_t spectrum[scope_width];
  static uint32_t signal_strength = 0;

  const uint8_t frequency_bin = (freq - 1000) * scope_width / 1500;
  const uint8_t Hz_1200 = (1200 - 1000) * scope_width / 1500;
  const uint8_t Hz_1500 = (1500 - 1000) * scope_width / 1500;
  const uint8_t Hz_2300 = (2300 - 1000) * scope_width / 1500;

  if (freq < 2450 && frequency_bin < scope_width) {
    spectrum[frequency_bin] = (spectrum[frequency_bin] * 15 + mag) / 16;
  }
  signal_strength = (signal_strength * 15 + mag) / 16;

  count++;
  if (count > 1000) {
    count = 0;

    display->drawRect(scope_x - 1, scope_y - 12, 13, scope_width + 3, COLOUR_WHITE);
    uint16_t waterfall[scope_width];
    for (uint16_t i = 0; i < scope_width; i++) {
      float scaled_dB = 2 * 20 * log10(spectrum[i]);
      uint16_t rounded_scaled_dB = std::max(std::min((unsigned int)scaled_dB, 255u), 0u);
      waterfall[i] = display->colour565(0, rounded_scaled_dB, rounded_scaled_dB);
    }

    waterfall[Hz_1200] = COLOUR_RED;
    waterfall[Hz_1500] = COLOUR_RED;
    waterfall[Hz_2300] = COLOUR_RED;
    display->writeHLine(scope_x, scope_y - 11 + row, scope_width, waterfall);
    display->dmaFlush();

    for (uint16_t i = 0; i < scope_width; i++) {
      spectrum[i] = 0;
    }
    row++;
    row &= 7;

    // Draw signal bar
    float scaled_dB = 2 * 20 * log10(signal_strength);
    uint16_t rounded_scaled_dB = std::max(std::min((unsigned int)scaled_dB, 149u), 0u);

    display->fillRect(scope_x, scope_y - 3, 3, rounded_scaled_dB, COLOUR_GREEN);
    display->fillRect(scope_x + scaled_dB, scope_y - 3, 3, 150u - rounded_scaled_dB, COLOUR_BLACK);
  }
}
