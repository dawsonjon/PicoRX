#include "eibi_utils.h"
#include "eibi.h"

#include "ctime"

bool is_active(s_frequency frequency)
{
  time_t now;
  time(&now);
  tm* t = gmtime(&now);
  uint16_t day_minute = t->tm_hour * 60 + t->tm_min;
  uint8_t weekday_flag = 1 << t->tm_wday;

  return (day_minute >= frequency.from) && (day_minute <= frequency.to) &&
         (weekday_flag & frequency.dayflags);
}

bool in_locations(s_locations* _locations, s_locations location, uint8_t num_locations)
{
  for (uint8_t i = 0; i < num_locations; i++)
    if (_locations[i].lon == location.lon && _locations[i].lat == location.lat)
      return true;

  return false;
}

bool is_valid_location(s_locations location)
{
  if (location.lon >= 180 && location.lon <= -180)
    return false;
  if (location.lat >= 90 && location.lat <= -90)
    return false;
  return true;
}

bool is_valid_id(int id)
{
  return id > 0;
}

bool get_nearest_active(uint16_t frequency_kHz, int16_t lon, int16_t lat,
                        s_frequency& nearest_frequency, float& nearest_distance)
{

  // lookup frequency in database
  int16_t from = -1;
  int16_t to = -1;
  int16_t id = -1;
  id = lookup_frequency(frequency_kHz, from, to);
  if (!is_valid_id(id))
    return false;

  nearest_distance = 1000000;
  nearest_frequency = {0};

  // make a condensed list of active and inactive transmitters on this frequency
  if (id >= 0) {
    for (int16_t idx = from; idx <= to; idx++) {

      bool active = is_active(frequencies[idx]);
      s_locations location = locations[frequencies[idx].transmitter_id];

      // if there is an active transmitter with an unknown location, display that
      if ((nearest_distance > 50000) && active) {
        nearest_distance = 0;
        nearest_frequency = frequencies[idx];
      }

      float distance = distance_km(location.lon, location.lat, lon, lat);
      if (active && is_valid_location(location) && distance < nearest_distance) {
        nearest_distance = distance;
        nearest_frequency = frequencies[idx];
      }
    }
  }

  return nearest_distance < 50000;
}
