#ifndef __EIBI_VIEW_H_
#define __EIBI_VIEW_H_

#include "eibi/spotter.h"

void draw_map(uint32_t frequency, int16_t lon, int16_t lat, c_spotter &spotter, ILI934X *display, bool force_redraw);
void draw_listing(uint32_t frequency, int16_t lon, int16_t lat, ILI934X *display, bool full_redraw, bool text_redraw);

#endif
