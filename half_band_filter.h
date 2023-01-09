#ifndef HALF_BAND_H
#define HALF_BAND_H

#include <stdint.h>

class half_band_filter
{
    private:
    static const uint8_t buf_size = 32u;
    int16_t bufi[32] = {0};
    int16_t bufq[32] = {0};
    uint8_t pointer = 0;
    public:
    half_band_filter();
    bool filter(int16_t &i, int16_t &q);

};

#endif
