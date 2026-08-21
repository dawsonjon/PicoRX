#ifndef __CW_DECODER_VIEW_H_
#define __CW_DECODER_VIEW_H_
#include "ili934x.h"
#include "codecs/cw_dsp.h"

class my_cw_dsp : public c_cw_dsp
{
  std::string decoded_text[NUM_CHANNELS];
  std::string partial_decoded_text[7];
  virtual void decode(uint16_t cluster, std::string text, std::string partial);
  ILI934X* display;
  public:
  void set_display(ILI934X* _display){display=_display;}
};

void draw_cw_decoder(ILI934X *display);
void draw_waterfall(c_cw_dsp &cw_dsp, ILI934X *display);
void draw_channel_status(my_cw_dsp &cw_dsp, ILI934X *display, uint32_t base_frequency);


#endif
