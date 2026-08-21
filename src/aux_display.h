#ifndef __waterfall_h__
#define __waterfall_h__
#include <cstdint>
#include "ili934x.h"
#include "xcvr.h"
#include "settings.h"
#include "codecs/sstv_decoder_picorx.h"
#include "eibi/spotter.h"
#include "codecs/cw_dsp.h"
#include "cw_decoder_view.h"



enum e_aux_display_state{waterfall_active, sstv_active, map_active, listing_active, cw_decoder_active};

class c_aux_display
{

  public:
  c_aux_display(xcvr &_transceiver, s_settings &_ui_settings, xcvr_settings &_settings, xcvr_status &_status);
  ~c_aux_display();
  void update(uint8_t spectrum[], uint8_t hold[], uint8_t dB10, uint8_t zoom);
  void configure_display(uint8_t settings, bool invert_colours, bool invert_tft, uint8_t display_driver);
  void powerOn(bool state);
  void draw();

  private:
  void update_spectrum(uint8_t spectrum[], uint8_t hold[], uint8_t dB10, uint8_t zoom, bool spectrum_hold);
  e_aux_display_state m_aux_display_state = waterfall_active;
  uint16_t heatmap(uint8_t value, bool lighten = false, bool highlight = false);
  uint16_t dBm_to_px(float power_dBm, int16_t px);
  float S_to_dBm(int S);
  int dBm_to_S(float power_dBm);
  uint8_t waterfall_buffer[120][256];
  ILI934X *display;
  xcvr &transceiver;
  s_settings &ui_settings;
  xcvr_settings &settings;
  xcvr_status &status;
  c_sstv_decoder_picorx sstv_decoder;
  c_spotter spotter;
  my_cw_dsp cw_dsp;
  bool enabled = false;
  bool power_state = true;
  bool refresh = true;
  void decode_sstv();
  void decode_cw();
  void update_map();
  void update_listing();

};

#endif
