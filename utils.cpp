#include "utils.h"
#include <cstdint>
#include <math.h>

int16_t sin_table[2048];

static const uint32_t GAIN = roundf(0.610334f * (1 << 16));

uint16_t rectangular_2_magnitude(int16_t i, int16_t q) {
  int16_t tempI;

  if (i < 0 && q >= 0) {
    i = std::abs(i);
  } else if (i < 0 && q < 0) {
    i = std::abs(i);
    q = std::abs(q);
  }

  for (uint16_t k = 0; k < 4; k++) {
    tempI = i;
    if (q > 0) {
      /* Rotate clockwise */
      i += (q >> k);
      q -= (tempI >> k);
    } else {
      /* Rotate counterclockwise */
      i -= (q >> k);
      q += (tempI >> k);
    }
  }
  uint16_t m = ((uint32_t)i * GAIN) >> 16;
  return m;
}

//from: https://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization/
//converted to fixed point
int16_t rectangular_2_phase(int16_t i, int16_t q)
{

   //handle condition where phase is unknown
   if(i==0 && q==0) return 0;

   const int16_t absi=i>0?i:-i;
   int16_t angle=0;
   if (q>=0)
   {
      //scale r so that it lies in range -8192 to 8192
      const int16_t r = ((int32_t)(q - absi) << 13) / (q + absi);
      angle = 8192 - r;
   }
   else
   {
      //scale r so that it lies in range -8192 to 8192
      const int16_t r = ((int32_t)(q + absi) << 13) / (absi - q);
      angle = (3 * 8192) - r;
   }

   //angle lies in range -32768 to 32767
   if (i < 0) return(-angle);     // negate if in quad III or IV
   else return(angle);
}

void initialise_luts()
{
  //pre-generate sin/cos lookup tables
  float scaling_factor = (1 << 15) - 1;
  for(uint16_t idx=0; idx<2048; idx++)
  {
    sin_table[idx] = roundf(sinf(2.0*M_PI*idx/2048.0) * scaling_factor);
  }
}
