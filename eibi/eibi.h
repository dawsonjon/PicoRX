

#ifndef __EIBI_H__
#define __EIBI_H__
#include <cstdint>

struct s_locations
{
  int16_t lat;
  int16_t lon;
};

struct s_frequency
{
  uint16_t frequency;
  uint16_t station_id;
  uint8_t country_id;
  uint16_t language_id;
  uint16_t transmitter_id;
  uint16_t from;
  uint16_t to;
  uint8_t dayflags;
};

static const uint16_t NUM_FREQUENCIES = 9391;
static const uint16_t NUM_STATIONS = 583;
static const uint16_t NUM_COUNTRIES = 96;
static const uint16_t NUM_LANGUAGES = 233;
static const uint16_t NUM_TRANSMITTERS = 492;

extern const char* const stations[NUM_STATIONS];
extern const char* const countries[NUM_COUNTRIES];
extern const char* const languages[NUM_LANGUAGES];
extern const char* const transmitters[NUM_TRANSMITTERS];
extern const s_frequency frequencies[NUM_FREQUENCIES];
extern const s_locations locations[NUM_TRANSMITTERS];

int16_t lookup_frequency(uint16_t frequency, int16_t& from, int16_t& to);
double distance_km(float lon_a, float lat_a, float lon_b, float lat_b);

#endif
