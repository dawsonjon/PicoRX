#ifndef HALF_BAND_H
#define HALF_BAND_H

#include "pico/stdlib.h"

class HalfBandFilter
{
    private:
    static const uint8_t num_taps = 27u;
    int16_t bufi[num_taps] = {0};
    int16_t bufq[num_taps] = {0};
    uint8_t pointer = 0;
    public:
    bool filter(int16_t &i, int16_t &q);

};

#endif
