#include "half_band_filter.h"

bool HalfBandFilter :: filter(int16_t &i, int16_t &q)
{

    static const int16_t kernel[num_taps] = {0, 0, -22, 0, 125, 0, -420, 0, 1121, 0, -2795, 0, 10183, 16384, 10183, 0, -2795, 0, 1121, 0, -420, 0, 125, 0, -22, 0, 0};

    bufi[pointer] = i;
    bufq[pointer] = q;
    pointer++;
    if(pointer == num_taps) pointer = 0;

    if(pointer & 1)
    {
        int32_t ii = 0;
        int32_t qq = 0;
        
        for(uint8_t tap=0; tap<num_taps; tap++)
        {
            ii += bufi[pointer] * kernel[tap];
            qq += bufq[pointer] * kernel[tap];
            pointer++;
            if(pointer == num_taps) pointer = 0;
        }

        i = ii >> 16;
        q = qq >> 16;

        return true;
    }
    return false;
}
