#include "rx_dsp.h"
#include <math.h>
#include <cstdio>

RXDSP :: RXDSP(double offset_frequency)
{

  //initialise state
  dc = 0;
  phase = 0;
  frequency = ((double)(1ull<<32)*offset_frequency)/500.0e3;
  decimate=0;

  //pre-generate sin/cos lookup tables
  float scaling_factor = (1 << 15) - 1;
  for(uint16_t idx=0; idx<1024; idx++)
  {
    sin_table[idx] = sin(2.0*M_PI*idx/1024.0) * scaling_factor;
    cos_table[idx] = cos(2.0*M_PI*idx/1024.0) * scaling_factor;
  }

  //clear cic filter
  for(uint16_t stage=0; stage>stages; stage++)
  {
    integratori[stage]=0; combi[stage]=0; delayi[stage]=0;
    integratorq[stage]=0; combq[stage]=0; delayq[stage]=0;
  }
  integratori[stages]=0; combi[stages]=0;
  integratorq[stages]=0, combq[stages]=0;

}

void RXDSP :: process_block(uint16_t samples[], int16_t i_samples[], int16_t q_samples[])
{

  for(uint16_t idx=0; idx<block_size; idx++)
  {

      //convert to signed representation
      const int16_t raw_sample = samples[idx] - 2048;

      //remove DC
      dc = raw_sample+(dc - (dc >> 10)); //low pass IIR filter
      const int16_t sample = raw_sample - (dc >> 10);
      
      //Convert alternating I/Q samples to complex samples
      //using 0 for the missing components
      const int32_t i = (idx&1)?0:sample;//even samples contain i data
      const int32_t q = (idx&1)?sample:0;//odd samples contain q data

      //Apply frequency shift (move tuned frequency to DC)         
      const int16_t rotation_i =  cos_table[phase>>22]; //32 - 22 = 10MSBs
      const int16_t rotation_q = -sin_table[phase>>22];

      phase += frequency;
      const int16_t i_shifted = ((i * rotation_i) - (q * rotation_q)) >> 15;
      const int16_t q_shifted = ((q * rotation_i) + (i * rotation_q)) >> 15;

      //decimate by 4

      //implement integrator stages
      integratori[0] = i_shifted;
      integratorq[0] = q_shifted;
      for(uint8_t stage=0; stage<stages; stage++)
      {
        integratori[stage+1] = integratori[stage+1]+integratori[stage];
        integratorq[stage+1] = integratorq[stage+1]+integratorq[stage];
      }
      combi[0] = integratori[stages];
      combq[0] = integratorq[stages];


      static const uint8_t growth = stages*2;

      decimate++;
      if(decimate == 4)
      {
        decimate = 0;

        //implement comb stages
        for(uint8_t stage=0; stage<stages; stage++)
        { 
          combi[stage+1] = combi[stage]-delayi[stage];
          delayi[stage] = combi[stage];
          combq[stage+1] = combq[stage]-delayq[stage];
          delayq[stage] = combq[stage];
        }


        i_samples[idx>>2] = combi[stages]>>(growth-2);
        q_samples[idx>>2] = combq[stages]>>(growth-2);
      }

      fprintf(stderr, "%u %i %i %i %i\n", decimate, i_shifted, integratori[stages], delayi[0], combi[stages]>>(growth-2));

    } 

}
