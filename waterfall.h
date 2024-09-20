#ifndef __waterfall_h__
#define __waterfall_h__
#include <cstdint>
#include "ili934x.h"
#include "rx.h"

class waterfall
{

  public:
  waterfall();
  ~waterfall();
  void update_spectrum(rx &receiver, rx_settings &settings, rx_status &status, uint8_t spectrum[], uint8_t dB10);
  void configure_display(uint8_t settings);

  private:
  uint16_t heatmap(uint8_t value, bool lighten = false, bool highlight = false);
  uint8_t waterfall_buffer[120][256];
  uint8_t *spectrum;
  ILI934X *display;
  bool enabled = false;

};

#endif
