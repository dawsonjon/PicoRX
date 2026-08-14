#ifndef __eibi_utils__
#define __eibi_utils__
#include "eibi.h"

bool is_active(s_frequency frequency);
bool in_locations(s_locations *_locations, s_locations location, uint8_t num_locations);
bool is_valid_location(s_locations location);
bool is_valid_id(int id);
bool get_nearest_active(uint16_t frequency_kHz, int16_t lon, int16_t lat, s_frequency &nearest_frequency, float &nearest_distance);

#endif
