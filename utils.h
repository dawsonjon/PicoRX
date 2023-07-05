#ifndef _utils_
#define _utils_

#include <cstdint>
#include <math.h>

int16_t sin_table[2048];

//from: http://dspguru.com/dsp/tricks/magnitude-estimator/
uint16_t rectangular_2_magnitude(int16_t i, int16_t q)
{
  //Measure magnitude
  const int16_t absi = i>0?i:-i;
  const int16_t absq = q>0?q:-q;
  return absi > absq ? absi + absq / 4 : absq + absi / 4;
}

float flt_rectangular_2_magnitude(float i, float q)
{
  //Measure magnitude
  const float absi = fabs(i);
  const float absq = fabs(q);
  return absi > absq ? ((absi * 0.947543636291) + (absq * 0.392485425092)) : ((absq * 0.947543636291) + (absi * 0.392485425092));
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
    sin_table[idx] = sin(2.0*M_PI*idx/2048.0) * scaling_factor;
  }

}

//bit reverse
static unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

uint8_t bit_reverse(uint8_t n) 
{
   // Reverse the top and bottom nibble then swap them.
   return (lookup[n&0b1111] << 4) | lookup[n>>4];
}

//calculate fft
void fft(float reals[], float imaginaries[]){

    int16_t stage, subdft_size, span, j, i, ip;
    float temp_real, temp_imaginary, imaginary_twiddle, real_twiddle;


    //bit reverse data
    for(i=0; i<256; i++){
        ip = bit_reverse(i);
        if(i < ip){
            temp_real = reals[i];
            temp_imaginary = imaginaries[i];
            reals[i] = reals[ip];
            imaginaries[i] = imaginaries[ip];
            reals[ip] = temp_real;
            imaginaries[ip] = temp_imaginary;
        }
    }

    //butterfly multiplies
    for(stage=0; stage<8; stage++){
        subdft_size = 2 << stage;
        span = subdft_size >> 1;

        for(j=0; j<span; j++){
            for(i=j; i<256; i+=subdft_size){
                ip=i+span;

                real_twiddle=cosf((float)j*M_PI/span);
                imaginary_twiddle=-sinf((float)j*M_PI/span);

                temp_real      = (reals[ip]*real_twiddle)      - (imaginaries[ip]*imaginary_twiddle);
                temp_imaginary = (reals[ip]*imaginary_twiddle) + (imaginaries[ip]*real_twiddle);

                reals[ip]       = reals[i]-temp_real;
                imaginaries[ip] = imaginaries[i]-temp_imaginary;

                reals[i]       = reals[i]+temp_real;
                imaginaries[i] = imaginaries[i]+temp_imaginary;

            }
        }

    }

}


#endif
