#include "eibi_view.h"
#include "eibi/eibi.h"
#include "eibi/eibi_utils.h"
#include "font_16x12.h"
#include "font_8x5.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

static void refresh_map(ILI934X *display) {
  (void) display;
}

void draw_map(uint32_t frequency, int16_t lon, int16_t lat, c_spotter &spotter, ILI934X *display, bool force_redraw)
{

  display->fillRect(0, 0, 20, 320, COLOUR_BLACK);
  display->fillRect(0, 220, 20, 320, COLOUR_BLACK);

  time_t now;
  time(&now);
  float lon_field = 340.0f;
  float lat_field = 160.0f;
  float start_lon = lon - lon_field/2;
  float start_lat = std::clamp(lat + lat_field/2, -90.0f+lat_field, 90.0f);
  spotter.set_view(start_lon, start_lat, lon_field, lat_field);
  spotter.draw_map(display, now, force_redraw);
  spotter.qth(display, lon, lat);

  //lookup frequency in database
  int16_t from = -1;
  int16_t to = -1;
  int16_t id = -1;
  id = lookup_frequency(frequency/1000, from, to);

  if(is_valid_id(id)) {
    for(int16_t idx = from; idx<=to; idx++){
      s_locations location = locations[frequencies[idx].transmitter_id];
      if(is_valid_location(location) && !is_active(frequencies[idx]))
        spotter.spot(display, location.lon, location.lat, false);
    }
    for(int16_t idx = from; idx<=to; idx++){
      s_locations location = locations[frequencies[idx].transmitter_id];
      if(is_valid_location(location) && is_active(frequencies[idx]))
        spotter.spot(display, location.lon, location.lat, true);
    }
  }

  float nearest_distance = 1000000;
  s_frequency nearest_frequency = {0};
  if(get_nearest_active(frequency/1000, lon, lat, nearest_frequency, nearest_distance)) {

    spotter.great_circle(display, lon, lat, locations[nearest_frequency.transmitter_id].lon, locations[nearest_frequency.transmitter_id].lat);

    uint16_t text_width = strlen(stations[nearest_frequency.station_id]) * 12;
    display->drawString((320-text_width)/2, 4, font_16x12, stations[nearest_frequency.station_id], COLOUR_WHITE, COLOUR_BLACK);

    char buff[50];
    snprintf(buff, 50, "%s - %s %.0fkm",
      countries[nearest_frequency.country_id],
      transmitters[nearest_frequency.transmitter_id],
      nearest_distance
    );
    text_width = strlen(buff) * 6;
    display->drawString((320-text_width)/2, 240-20, font_8x5, buff, COLOUR_WHITE, COLOUR_BLACK);

    char weekdays[]="SMTWTFS";
    for(uint8_t d=0; d<7; d++) {
      if(!(nearest_frequency.dayflags & (1<<d))){
        weekdays[d] = '-';
      }
    }
    snprintf(buff, 50, "%s %7s %02u:%02u-%02u:%02u",
      languages[nearest_frequency.language_id],
      weekdays,
      nearest_frequency.from/60,
      nearest_frequency.from%60,
      nearest_frequency.to/60,
      nearest_frequency.to%60
    );
    text_width = strlen(buff) * 6;
    display->drawString((320-text_width)/2, 240-10, font_8x5, buff, COLOUR_WHITE, COLOUR_BLACK);

  }

  refresh_map(display);
}

static const char* scroll(const char *string, uint8_t display_len, uint8_t phase){
  static char wrapped_scroll_buff[51];
  uint8_t length = strlen(string);
  for(uint8_t i=0; i<display_len; i++){
    wrapped_scroll_buff[i] = string[(i+phase)%length];
  }
  wrapped_scroll_buff[50]=0;
  return wrapped_scroll_buff;
}

void draw_listing(uint32_t frequency, int16_t lon, int16_t lat, ILI934X *display, bool full_redraw, bool text_redraw)
{

  static uint8_t phase = 0;

  if(full_redraw) {
    display->fillRect(7, 2, 11, 306, display->colour565(128, 128, 128));
    display->fillRect(0, 0, 240, 320, display->colour565(200, 200, 200));
    display->drawRect(7, 15, 220, 306, display->colour565(128, 128, 128));
    display->fillRect(7, 2, 11, 306, display->colour565(128, 128, 128));
    display->drawString(10, 3, font_8x5, "Time UTC, Station, Country, Lang, Site, km, Days", COLOUR_BLACK, display->colour565(128, 128, 128));
  }
  if(text_redraw) {
    display->fillRect(7, 15, 220, 306, display->colour565(255, 255, 255));
  }

  //lookup frequency in database
  int16_t from = -1;
  int16_t to = -1;
  int16_t id = -1;
  id = lookup_frequency(frequency/1000, from, to);

  if(is_valid_id(id)) {
    uint8_t count = 0;
    for(int16_t idx = from; idx<=to; idx++){
      s_locations location = locations[frequencies[idx].transmitter_id];

      char weekdays[]="SMTWTFS";
      for(uint8_t d=0; d<7; d++) {
        if(!(frequencies[idx].dayflags & (1<<d))){
          weekdays[d] = '-';
        }
      }

      char scroll_buff[100];
      snprintf(scroll_buff, 100, "%s %s %s %s %.0f km %s -- ",
          stations[frequencies[idx].station_id],
          countries[frequencies[idx].country_id],
          languages[frequencies[idx].language_id],
          transmitters[frequencies[idx].transmitter_id],
          distance_km(location.lon, location.lat, lon, lat),
          weekdays
      );

      char buff[52];
      snprintf(buff, 52, "%02u:%02u-%02u:%02u %-38.38s",
          frequencies[idx].from/60%24,
          frequencies[idx].from%60,
          frequencies[idx].to/60%24,
          frequencies[idx].to%60,
          scroll(scroll_buff, 40, phase));

      uint16_t colour = is_active(frequencies[idx])?COLOUR_RED:COLOUR_BLUE;
      display->drawString(10, 20+count++*10, font_8x5, buff, colour, COLOUR_WHITE);
      if(count > 20) break;
    }
  }

  phase++;

}
