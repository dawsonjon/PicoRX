#ifndef __SSTV_DECODER_PICORX__
#define __SSTV_DECODER_PICORX__
#include <cstdint>
#include "../src/ili934x.h"
#include "../src/xcvr.h"
#include "sstv_decoder.h"

class c_sstv_decoder_picorx : public c_sstv_decoder
{

  ILI934X *display;
  xcvr &receiver;
  uint16_t row_number = 0;
  static const uint16_t display_width = 320;
  static const uint16_t display_height = 240 - 20; //allow space for status bar
  int16_t get_audio_sample() {return 0;}
  void get_iq_sample(int16_t &i, int16_t &q) { receiver.get_raw_data(i, q); }
  void image_write_line(uint16_t line_rgb565[], uint16_t y, uint16_t width, uint16_t height, const char* mode_string);
  void scope(uint16_t mag, int16_t freq);

  public:

  void reset(){row_number = 9;}
  void set_display(ILI934X *_display) {display = _display;};
  c_sstv_decoder_picorx(xcvr &_receiver) : c_sstv_decoder{15000}, receiver(_receiver){}

};

#endif
