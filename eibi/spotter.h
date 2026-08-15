#ifndef __SPOTTER__H_
#define __SPOTTER__H_

#include "../src/ili934x.h"
#include "flags.h"
#include <ctime>

class c_spotter
{
  float m_view_lon;
  float m_view_lat;
  float m_field_lon;
  float m_field_lat;
  void lat_lon_to_pixel(uint16_t& x, uint16_t& y, float lon, float lat);
  void map_to_lat_lon(uint16_t x, uint16_t y, float& lon, float& lat);
  // float is_night(uint16_t x, uint16_t y, float sun_lon, float sun_lat);
  float map_y_to_lat(uint16_t y);
  float map_x_to_lon(uint16_t x);

  uint16_t dirty_min_y = UINT16_MAX;
  uint16_t dirty_max_y = 0;

public:
  double distance_km(float lon_a, float lat_a, float lon_b, float lat_b);
  void set_view(float view_lon, float view_lat, float field_lon, float field_lat);
  void great_circle(ILI934X* display, float lon1, float lat1, float lon2, float lat2);
  void spot(ILI934X* display, float lon, float lat, bool active);
  void qth(ILI934X* display, float lon, float lat);
  void draw_map(ILI934X* display, std::time_t t, bool force_redraw = false);
};

#endif
