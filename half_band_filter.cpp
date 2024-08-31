#include "half_band_filter.h"
#include <cstdio>

half_band_filter :: half_band_filter()
{
  pointer = 0;
  for(uint8_t tap=0; tap < buf_size; tap++)
  {
    bufi[tap] = 0;
    bufq[tap] = 0;
  }
}

bool half_band_filter :: filter(int16_t &i, int16_t &q)
{

    //filter kernel:
    //0, 0, -22, 0, 125, 0, -420, 0, 1121, 0, -2795, 0, 10183, 16384, 10183, 0, -2795, 0, 1121, 0, -420, 0, 125, 0, -22, 0, 0

    bufi[pointer] = i;
    bufq[pointer] = q;
    pointer++;
    pointer&=0x1f;

    if(pointer & 1)
    {

        const uint8_t idx2  = (pointer+2 ) & 0x1f;
        const uint8_t idx4  = (pointer+4 ) & 0x1f;
        const uint8_t idx6  = (pointer+6 ) & 0x1f;
        const uint8_t idx8  = (pointer+8 ) & 0x1f;
        const uint8_t idx10 = (pointer+10) & 0x1f;
        const uint8_t idx12 = (pointer+12) & 0x1f;
        const uint8_t idx13 = (pointer+13) & 0x1f;
        const uint8_t idx14 = (pointer+14) & 0x1f;
        const uint8_t idx16 = (pointer+16) & 0x1f;
        const uint8_t idx18 = (pointer+18) & 0x1f;
        const uint8_t idx20 = (pointer+20) & 0x1f;
        const uint8_t idx22 = (pointer+22) & 0x1f;
        const uint8_t idx24 = (pointer+24) & 0x1f;
        // const uint8_t idx26 = pointer+26 & 0x1f;
        
        i = (
          //(bufi[pointer]  + bufi[idx26]) * 0) + 
          (((int32_t)bufi[idx2]     + (int32_t)bufi[idx24]) * -22) +
          (((int32_t)bufi[idx4]     + (int32_t)bufi[idx22]) * 125) +
          (((int32_t)bufi[idx6]     + (int32_t)bufi[idx20]) * 420) +
          (((int32_t)bufi[idx8]     + (int32_t)bufi[idx18]) * 1121) +
          (((int32_t)bufi[idx10]    + (int32_t)bufi[idx16]) * -2795) +
          (((int32_t)bufi[idx12]    + (int32_t)bufi[idx14]) * 10183) +
          (((int32_t)bufi[idx13]                 ) * 16384)) >> 15;

        q = (
          //(bufq[pointer]  + bufq[idx26]) * 0) + 
          (((int32_t)bufq[idx2]     + (int32_t)bufq[idx24]) * -22) +
          (((int32_t)bufq[idx4]     + (int32_t)bufq[idx22]) * 125) +
          (((int32_t)bufq[idx6]     + (int32_t)bufq[idx20]) * 420) +
          (((int32_t)bufq[idx8]     + (int32_t)bufq[idx18]) * 1121) +
          (((int32_t)bufq[idx10]    + (int32_t)bufq[idx16]) * -2795) +
          (((int32_t)bufq[idx12]    + (int32_t)bufq[idx14]) * 10183) +
          (((int32_t)bufq[idx13]                 ) * 16384)) >> 15;

        return true;
    }
    return false;
}
