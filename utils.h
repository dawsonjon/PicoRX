#ifndef _utils_
#define _utils_

int16_t sin_table[1024];
int16_t cos_table[1024];

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

void initialise_luts()
{
  //pre-generate sin/cos lookup tables
  float scaling_factor = (1 << 15) - 1;
  for(uint16_t idx=0; idx<1024; idx++)
  {
    sin_table[idx] = sin(2.0*M_PI*idx/1024.0) * scaling_factor;
    cos_table[idx] = cos(2.0*M_PI*idx/1024.0) * scaling_factor;
  }
}

//bit reverse
unsigned bit_reverse(unsigned forward, uint16_t m){
    unsigned reversed=0;
    unsigned i;
    for(i=0; i<m; i++){
        reversed <<= 1;
        reversed |= forward & 1;
        forward >>= 1;
    }
    return reversed;
}

//calculate fft
void fft(uint16_t n, uint16_t m, int16_t reals[], int16_t imaginaries[], int16_t sin_lookup[], int16_t cos_lookup[]){

    int16_t stage, subdft_size, span, i, ip, j;
    int16_t temp_real, temp_imaginary, imaginary_twiddle, real_twiddle;


    //bit reverse data
    for(i=0; i<n; i++){
        ip = bit_reverse(i, m);
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
    for(stage=0; stage<m; stage++){
        subdft_size = 2 << stage;
        span = subdft_size >> 1;

        for(j=0; j<span; j++){
            for(i=j; i<n; i+=subdft_size){
                ip=i+span;

                real_twiddle=cos_lookup[j*512u>>stage];
                imaginary_twiddle=-sin_lookup[j*512u>>stage];

                temp_real      = (((int32_t)reals[ip]*(int32_t)real_twiddle)      - ((int32_t)imaginaries[ip]*(int32_t)imaginary_twiddle)) >> 15;
                temp_imaginary = (((int32_t)reals[ip]*(int32_t)imaginary_twiddle) + ((int32_t)imaginaries[ip]*(int32_t)real_twiddle)) >> 15;

                reals[ip]       = reals[i]-temp_real;
                imaginaries[ip] = imaginaries[i]-temp_imaginary;

                reals[i]       = reals[i]+temp_real;
                imaginaries[i] = imaginaries[i]+temp_imaginary;

            }
        }

    }

}


#endif
