#ifndef _utils_
#define _utils_

//from: http://dspguru.com/dsp/tricks/magnitude-estimator/
uint16_t rectangular_2_magnitude(int16_t i, int16_t q)
{
  //Measure magnitude
  const int16_t absi = i>0?i:-i;
  const int16_t absq = q>0?q:-q;
  return absi > absq ? absi + absq / 4 : absq + absi / 4;
}

//from: https://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization/
//converted to fixed point
uint16_t rectangular_2_phase(int16_t i, int16_t q)
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
      angle = 3 * 8192 - r;
   }

   //angle lies in range -32768 to 32767
   if (q < 0)
   return(-angle);     // negate if in quad III or IV
   else
   return(angle);
}

#endif
