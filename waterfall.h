#ifndef __waterfall_h__
#define __waterfall_h__
#include <cstdint>
#include "ili934x.h"
#include "fft_filter.h"

class waterfall
{

  public:
  waterfall();
  ~waterfall();
  void new_spectrum(uint8_t spectrum[], s_filter_control &fc);

  private:
  uint16_t heatmap(uint8_t value, bool lighten = false);
  uint8_t waterfall_buffer[120][256];
  ILI934X *display;

};

#endif
