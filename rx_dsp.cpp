#include "rx_dsp.h"
#include <math.h>
#include <cstdio>


uint16_t rx_dsp :: process_block(uint16_t samples[], int16_t audio_samples[])
{

  uint16_t odx = 0;
  for(uint16_t idx=0; idx<adc_block_size; idx++)
  {

      //convert to signed representation
      const int16_t raw_sample = samples[idx] - (1<<(adc_bits-1));

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
      //CIC (20)    12.5kHz      +/-6.25kHz
      //filt1       15.625kHz    +/-3.125kHz(with aliases outside)
      //filt2       15.625kHz    +/-3.125kHz(free from aliases)

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


          //Demodulate
          const int32_t absi = decimated_i > 0?decimated_i:-decimated_i;
          const int32_t absq = decimated_q > 0?decimated_q:-decimated_q;
          int32_t audio = absi > absq ? absi + absq / 4 : absq + absi / 4;

          //remove DC
          audio_dc = audio+(audio_dc - (audio_dc >> 5)); //low pass IIR filter
          audio -= (audio_dc >> 5);

          //Audio AGC
          if(true)
          {

            //Use a leaky max hold to estimate audio power
            static const uint8_t extra_bits = 16;
            const int32_t audio_extended = audio << extra_bits;

            if(audio_extended > max_hold)
            {
              //attack
              max_hold += (audio_extended - max_hold) >> attack_factor;
              hang_timer = hang_time;
            }
            else if(hang_timer)
            {
              //hang
              hang_timer--;
            }
            else if(max_hold > 0)
            {
              //decay
              max_hold -= max_hold>>decay_factor; 
            }

            //calculate gain 
            const int16_t magnitude = max_hold >> extra_bits;
            const int16_t limit = INT16_MAX; //hard limit
            const int16_t setpoint = limit/2; //about half full scale

            //apply gain
            if(magnitude > 0)
            {
              const int16_t gain = setpoint/magnitude;
              audio *= gain;
            }

            //soft clip (compress)
            if (audio > setpoint)  audio =  setpoint + ((audio-setpoint)>>1);
            if (audio < -setpoint) audio = -setpoint - ((audio+setpoint)>>1);

            //hard clamp
            if (audio > limit)  audio = limit;
            if (audio < -limit) audio = -limit;

            //convert to unsigned
            audio += limit;
            audio /= pwm_scale; //scale to PWM ceil(32768/250)

            for(uint8_t sample=0; sample < interpolation_rate; sample++)
            {
              audio_samples[odx] = audio;
              odx++;
            }

            
          }


        }
      }
    } 
    return odx;
}

rx_dsp :: rx_dsp()
{

  //initialise state
  dc = 0;
  phase = 0;
  decimate=0;
  frequency=0;

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

  set_agc_speed(3);


}

void rx_dsp :: set_agc_speed(uint8_t agc_setting)
{
  //Configure AGC
  // input fs=500000.000000 Hz
  // decimation=20 x 2
  // fs=12500.000000 Hz
  // Setting Decay Time(s) Factor Attack Time(s) Factor  Hang  Timer
  // ======= ============= ====== ============== ======  ====  =====
  // fast        0.047       9        0.001         2    0.1s   1250
  // medium      0.189       10       0.001         2    0.25s  3125
  // slow        0.377       11       0.001         2    1s     12500
  // long        1.509       13       0.001         2    2s     25000


  switch(agc_setting)
  {
      case 0: //fast
        attack_factor=2;
        decay_factor=9;
        hang_time=1250;
        break;

      case 1: //medium
        attack_factor=2;
        decay_factor=10;
        hang_time=3125;
        break;

      case 2: //slow
        attack_factor=2;
        decay_factor=11;
        hang_time=12500;
        break;

      default: //long
        attack_factor=2;
        decay_factor=13;
        hang_time=25000;
        break;
  }
}

void rx_dsp :: set_frequency_offset_Hz(double offset_frequency)
{
  frequency = ((double)(1ull<<32)*offset_frequency)/adc_sample_rate;
}
