#include "rx_dsp.h"
#include <math.h>
#include <cstdio>

rx_dsp :: rx_dsp(double offset_frequency)
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
  integratori1=0; integratorq1=0;
  integratori2=0; integratorq2=0;
  integratori3=0; integratorq3=0;
  integratori4=0; integratorq4=0;
  delayi0=0; delayq0=0;
  delayi1=0; delayq1=0;
  delayi2=0; delayq2=0;
  delayi3=0; delayq3=0;

}

uint16_t rx_dsp :: process_block(uint16_t samples[], int16_t i_samples[], int16_t q_samples[])
{

  uint16_t odx = 0;
  for(uint16_t idx=0; idx<block_size; idx++)
  {

      //convert to signed representation
      const int16_t raw_sample = samples[idx] - 2048;

      //remove DC
      dc = raw_sample+(dc - (dc >> 10)); //low pass IIR filter
      const int16_t sample = raw_sample - (dc >> 10);
      
      //Apply frequency shift (move tuned frequency to DC)         
      const int32_t i = (idx&1)?0:sample;//even samples contain i data
      const int32_t q = (idx&1)?sample:0;//odd samples contain q data

      //Apply frequency shift (move tuned frequency to DC)         
      const int16_t rotation_i =  cos_table[phase>>22]; //32 - 22 = 10MSBs
      const int16_t rotation_q = -sin_table[phase>>22];

      phase += frequency;
      const int16_t i_shifted = ((i * rotation_i) - (q * rotation_q)) >> 15;
      const int16_t q_shifted = ((q * rotation_i) + (i * rotation_q)) >> 15;

      //Decimate

      //             fs          Alias Free
      //raw data    500kHz
      //CIC (16)    31.25kHz     +/-7kHz
      //filt1       15.625kHz    +/-4.5(with aliases outside)
      //filt2       15.625kHz    +/-4.5(free from aliases)


      //CIC decimation filter
      //implement integrator stages
      integratori1 += i_shifted;
      integratorq1 += q_shifted;
      integratori2 += integratori1;
      integratorq2 += integratorq1;
      integratori3 += integratori2;
      integratorq3 += integratorq2;
      integratori4 += integratori3;
      integratorq4 += integratorq3;

      //fprintf(stderr, "num_output samples: %i %i %i\n", idx, odx, decimate);

      decimate++;
      if(decimate == decimation_rate)
      {
        decimate = 0;

        //implement comb stages
        const int32_t combi1 = integratori4-delayi0;
        const int32_t combq1 = integratorq4-delayq0;
        const int32_t combi2 = combi1-delayi1;
        const int32_t combq2 = combq1-delayq1;
        const int32_t combi3 = combi2-delayi2;
        const int32_t combq3 = combq2-delayq2;
        const int32_t combi4 = combi3-delayi3;
        const int32_t combq4 = combq3-delayq3;
        delayi0 = integratori4;
        delayq0 = integratorq4;
        delayi1 = combi1;
        delayq1 = combq1;
        delayi2 = combi2;
        delayq2 = combq2;
        delayi3 = combi3;
        delayq3 = combq3;

        int16_t decimated_i = combi4>>(growth-2);
        int16_t decimated_q = combq4>>(growth-2);
        
        //first half band decimating filter
        bool new_sample = half_band_filter_inst.filter(decimated_i, decimated_q);
        //second half band decimating filter
        if(new_sample) new_sample = half_band_filter2_inst.filter(decimated_i, decimated_q);
        if(new_sample)
        {

          const int16_t max = decimated_i > decimated_q?decimated_i:decimated_q;
          const int16_t min = decimated_i < decimated_q?decimated_i:decimated_q;
          decimated_i = ((max * 61) / 64) + ((min * 13) / 32);

          i_samples[odx] = decimated_i;
          //q_samples[odx] = decimated_q;
          odx++;
        }
      }


    } 
    return odx;

}
