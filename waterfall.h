#ifndef __waterfall_h__
#define __waterfall_h__
#include <cstdint>
#include "ili934x.h"

class waterfall
{

  public:
  waterfall();
  ~waterfall();
  void new_spectrum(float spectrum[]);

  private:
  void heatmap(uint8_t value, uint16_t &color);
  uint8_t waterfall_buffer[120][256];
  ILI934X *display;

};

#endif
