#ifndef HALF_BAND_2_H
#define HALF_BAND_2_H

#include <stdint.h>

class half_band_filter2
{
    private:
    static const uint8_t buf_size = 64u;
    int16_t bufi[buf_size] = {0};
    int16_t bufq[buf_size] = {0};
    uint8_t pointer = 0;
    public:
    half_band_filter2();
    bool filter(int16_t &i, int16_t &q);

};

#endif
