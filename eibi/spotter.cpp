#include "spotter.h"
#include "../src/ili934x.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>

#define LOGGING
#ifdef LOGGING
#ifdef ARDUINO
#include <Arduino.h>
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#include <cstdio>
#define DEBUG_PRINTF(...) std::printf(__VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(...)
#endif

static float deg2rad(float d)
{
  return d * M_PI / 180.0;
}
static float rad2deg(float r)
{
  return r * 180.0 / M_PI;
}

#include "Blue_Marble_2002_320x160.h"
const uint16_t map_width = 330;
const uint16_t map_height = 165;
const uint16_t display_width = 320;
const uint16_t display_height = 200;
const uint16_t display_x = 0;
const uint16_t display_y = 20;

void c_spotter::set_view(float view_lon, float view_lat, float field_lon, float field_lat)
{
  m_view_lon = view_lon;
  m_view_lat = view_lat;
  m_field_lon = field_lon;
  m_field_lat = field_lat;
}

void c_spotter::lat_lon_to_pixel(uint16_t& x, uint16_t& y, float lon, float lat)
{
  float lon_separation = (lon - m_view_lon);
  if (lon_separation >= 360.0f)
    lon_separation -= 360.0f;
  if (lon_separation < 0.0f)
    lon_separation += 360.0f;
  x = display_x + display_width * (lon_separation / m_field_lon);
  y = display_y + display_height * ((m_view_lat - lat) / m_field_lat);
}

void c_spotter::map_to_lat_lon(uint16_t x, uint16_t y, float& lon, float& lat)
{
  lon = (360.0f * x / map_width) - 180.0f;
  lat = 90.0f - (180.0f * y / map_height);
}

float c_spotter::map_y_to_lat(uint16_t y)
{
  return 90.0f - (180.0f * y / map_height);
}
float c_spotter::map_x_to_lon(uint16_t x)
{
  float lon = (360.0f * x / map_width) - 180.0f;
  if (lon > 180)
    lon -= 360;
  if (lon < -180)
    lon += 360;
  return lon;
}

// Convert lat/lon to 3D unit vector
static void to_xyz(float lon, float lat, float& x, float& y, float& z)
{
  lat = deg2rad(lat);
  lon = deg2rad(lon);

  x = cos(lat) * cos(lon);
  y = cos(lat) * sin(lon);
  z = sin(lat);
}

// Convert 3D unit vector back to lat/lon
static void to_latlon(float& lon, float& lat, float x, float y, float z)
{
  lat = rad2deg(atan2(z, sqrt(x * x + y * y)));
  lon = rad2deg(atan2(y, x));
}

void c_spotter::spot(ILI934X* display, float lon, float lat, bool active)
{
  uint16_t pixel_x, pixel_y;
  lat_lon_to_pixel(pixel_x, pixel_y, lon, lat);
  dirty_min_y = std::min((uint16_t)(pixel_y - 3), dirty_min_y);
  dirty_max_y = std::max((uint16_t)(pixel_y + 3), dirty_max_y);
  display->fillCircle(pixel_x, pixel_y, 3, active ? COLOUR_RED : COLOUR_GREY);
  display->drawCircle(pixel_x, pixel_y, 3, COLOUR_WHITE);
}

void c_spotter::qth(ILI934X* display, float lon, float lat)
{
  uint16_t pixel_x, pixel_y;
  lat_lon_to_pixel(pixel_x, pixel_y, lon, lat);
  display->fillCircle(pixel_x, pixel_y, 3, COLOUR_BLUE);
  display->drawCircle(pixel_x, pixel_y, 3, COLOUR_WHITE);
}

static void subsolar_point(std::time_t t, float& lat, float& lon)
{
  // Convert to UTC broken-down time
  struct tm* gmt = gmtime(&t);

  // Day of year (tm_yday is 0–365, so add 1)
  int N = gmt->tm_yday + 1;

  // UTC time in fractional hours
  float utc_hours = gmt->tm_hour + gmt->tm_min / 60.0f + gmt->tm_sec / 3600.0f;

  // --- Subsolar latitude (declination) ---
  lat = 23.44f * sin(2 * M_PI * (N - 81.0f) / 365.0);

  // --- Subsolar longitude ---
  lon = 180.0 - 15.0 * utc_hours;

  // Wrap to [-180, 180]
  if (lon > 180)
    lon -= 360;
  if (lon < -180)
    lon += 360;
}

void c_spotter::great_circle(ILI934X* display, float lon1, float lat1, float lon2, float lat2)
{

  // Convert endpoints to 3D
  float x1, y1, z1;
  float x2, y2, z2;
  to_xyz(lon1, lat1, x1, y1, z1);
  to_xyz(lon2, lat2, x2, y2, z2);

  // Angle between them
  float dot = x1 * x2 + y1 * y2 + z1 * z2;
  dot = std::fmax(-1.0, std::fmin(1.0, dot)); // clamp
  float omega = acos(dot);
  float sin_omega = sin(omega);

  uint16_t pixel_x, pixel_y;
  uint16_t last_x, last_y;
  lat_lon_to_pixel(last_x, last_y, lon1, lat1);
  display->fillCircle(last_x, last_y, 2, COLOUR_BLUE);
  display->drawCircle(last_x, last_y, 2, COLOUR_WHITE);
  dirty_min_y = std::min((uint16_t)(last_y - 2), dirty_min_y);
  dirty_max_y = std::max((uint16_t)(last_y + 2), dirty_max_y);

  const int num_points = 10;
  for (int i = 0; i < num_points; i++) {
    float t = (float)i / (num_points - 1); // 0 → 1

    float w1 = sin((1 - t) * omega) / sin_omega;
    double w2 = sin(t * omega) / sin_omega;

    float x = w1 * x1 + w2 * x2;
    float y = w1 * y1 + w2 * y2;
    float z = w1 * z1 + w2 * z2;

    // Normalize (important for numerical stability)
    float norm = sqrt(x * x + y * y + z * z);
    x /= norm;
    y /= norm;
    z /= norm;

    float lon, lat;
    to_latlon(lon, lat, x, y, z);
    lat_lon_to_pixel(pixel_x, pixel_y, lon, lat);
    display->drawLine(last_x, last_y, pixel_x, pixel_y, COLOUR_AQUA);
    dirty_min_y = std::min((uint16_t)(pixel_y - 2), dirty_min_y);
    dirty_max_y = std::max((uint16_t)(pixel_y + 2), dirty_max_y);

    last_x = pixel_x;
    last_y = pixel_y;
  }
  display->fillCircle(pixel_x, pixel_y, 2, COLOUR_RED);
  display->drawCircle(pixel_x, pixel_y, 2, COLOUR_WHITE);
  dirty_min_y = std::min((uint16_t)(pixel_y - 2), dirty_min_y);
  dirty_max_y = std::max((uint16_t)(pixel_y + 2), dirty_max_y);
}

static uint16_t shade_rgb565_night(uint16_t pixel, uint8_t shade)
{
  uint16_t p = (pixel >> 8) | (pixel << 8);

  uint8_t r = (p >> 11) & 0x1F;
  uint8_t g = (p >> 5) & 0x3F;
  uint8_t b = (p >> 0) & 0x1F;

  // Darken
  r = (r * shade) >> 8;
  g = (g * shade) >> 8;
  b = (b * shade) >> 8;

  // Add slight blue tint
  b = std::min<uint8_t>(31, b + 2);

  uint16_t out = (r << 11) | (g << 5) | (b);

  return (out >> 8) | (out << 8);
}

void c_spotter::draw_map(ILI934X* display, std::time_t t, bool force_redraw)
{

  static bool drawn = false;
  if (force_redraw)
    drawn = false;

  float sun_lat, sun_lon;
  subsolar_point(t, sun_lat, sun_lon);
  float tan_sun_lat = tan(deg2rad(sun_lat));

  int16_t view_x = (map_width * ((m_view_lon + 180.0f) / 360.0f));
  int16_t view_y = (map_height * ((90.0f - m_view_lat) / 180.0f));
  uint16_t view_width = (map_width * m_field_lon / 360.0f);
  uint16_t view_height = (map_height * m_field_lat / 180.0f);

  for (uint16_t yy = 0; yy < display_height; ++yy) {
    if ((display_y + yy < dirty_min_y || display_y + yy > dirty_max_y) && drawn)
      continue;

    uint16_t map_y = view_y + (yy * view_height / display_height);
    float lat = map_y_to_lat(map_y);
    float v = -tan(deg2rad(lat)) * tan_sun_lat;
    float delta = acos(v);
    float lon1 = deg2rad(sun_lon) - delta;
    float lon2 = deg2rad(sun_lon) + delta;
    if (lon1 > M_PI)
      lon1 -= 2 * M_PI;
    if (lon1 < -M_PI)
      lon1 += 2 * M_PI;
    if (lon2 > M_PI)
      lon2 -= 2 * M_PI;
    if (lon2 < -M_PI)
      lon2 += 2 * M_PI;

    uint16_t line[display_width];
    for (uint16_t xx = 0; xx < display_width; ++xx) {
      int16_t map_x = view_x + (xx * view_width / display_width);
      if (map_x < 0)
        map_x += map_width;
      if (map_x >= map_width)
        map_x -= map_width;
      if (map_y < map_height) {
        float lon = deg2rad(map_x_to_lon(map_x));
        if (v < -1.0f) {
          line[xx] = shade_rgb565_night(Blue_Marble_2002_320x160[(map_y * map_width) + map_x], 255);
        } else if (v >= 1.0f) {
          line[xx] = shade_rgb565_night(Blue_Marble_2002_320x160[(map_y * map_width) + map_x], 180);
        } else if (lon2 > lon1 && lon >= lon1 && lon <= lon2) {
          line[xx] = shade_rgb565_night(Blue_Marble_2002_320x160[(map_y * map_width) + map_x], 255);
        } else if (lon1 > lon2 && (lon > lon1 || lon < lon2)) {
          line[xx] = shade_rgb565_night(Blue_Marble_2002_320x160[(map_y * map_width) + map_x], 255);
        } else {
          line[xx] = shade_rgb565_night(Blue_Marble_2002_320x160[(map_y * map_width) + map_x], 180);
        }
      } else {
        line[xx] = COLOUR_BLUE;
      }
    }
    display->dmaFlush();
    display->writeHLine(display_x, display_y + yy, display_width, line);
  }
  display->dmaFlush();

  drawn = true;
  dirty_min_y = UINT16_MAX;
  dirty_max_y = 0;
}
