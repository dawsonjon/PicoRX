#include "rx_dsp.h"
#include "rx_definitions.h"
#include "utils.h"
#include <math.h>
#include <cstdio>
#include "pico/stdlib.h"

uint16_t __not_in_flash_func(rx_dsp :: process_block)(uint16_t samples[], int16_t audio_samples[])
{

  uint16_t odx = 0;
  int32_t magnitude_sum = 0;
  int32_t sample_accumulator = 0;

  //if the capture buffer isn't in use, fill it
  if(!capture_data) 
  {
    capture_data = sem_try_acquire(&spectrum_semaphore);
    cap = 0;
  }

  printf("frame\n");

  for(uint16_t idx=0; idx<adc_block_size; idx++)
  {
      //convert to signed representation
      const int16_t raw_sample = samples[idx];// - (1<<(adc_bits-1));
      sample_accumulator += raw_sample;

      //remove dc
      const int16_t sample = raw_sample - dc;

      //work out which samples are i and q
      int16_t i = (idx&1^1)*raw_sample;//even samples contain i data
      int16_t q = (idx&1)*raw_sample;//odd samples contain q data

      //Apply frequency shift (move tuned frequency to DC)         
      frequency_shift(i, q);

      //decimate by factor of 40
      if(decimate(i, q))
      {

        //capture data for spectrum
        if(capture_data)
        {
          capture_i[cap] = i;
          capture_q[cap] = q;
          cap++;
          if(cap == 1024)
          {
            sem_release(&spectrum_semaphore);
            capture_data = false;
          }
        }


        //Measure amplitude (for signal strength indicator)
        int32_t amplitude = rectangular_2_magnitude(i, q);
        magnitude_sum += amplitude;

        //Demodulate to give audio sample
        int32_t audio = demodulate(i, q);

        //Automatic gain control scales signal to use full 16 bit range
        //e.g. -32767 to 32767
        audio = automatic_gain_control(audio);

        //digital volume control
        audio = ((int32_t)audio * gain_numerator) >> 8;

        //squenlch
        if(signal_amplitude < squelch_threshold)
        {
          audio = 0;
        }

        //convert to unsigned value in range 0 to 500 to output to PWM
        audio += INT16_MAX;
        audio /= pwm_scale; 

        for(uint8_t sample=0; sample < interpolation_rate; sample++)
        {
          audio_samples[odx] = audio;
          odx++;
        }
          
      }
    } 

    //average over the number of samples
    signal_amplitude = (magnitude_sum * decimation_rate * 2)/adc_block_size;
    dc = sample_accumulator/adc_block_size;

    return odx;
}

void __not_in_flash_func(rx_dsp :: frequency_shift)(int16_t &i, int16_t &q)
//void rx_dsp :: frequency_shift(int16_t &i, int16_t &q)
{
    static const int16_t k = (1<<(15-1));

    //Apply frequency shift (move tuned frequency to DC)         
    //dither = 1664525u*dither + 1013904223u;
    //const uint16_t dithered_phase = (phase + (dither >> 29) >> 22);
    const uint16_t dithered_phase = (phase >> 21);
    const int16_t rotation_i =  sin_table[(dithered_phase+521u) & 0x7ff]; //32 - 21 = 11MSBs
    const int16_t rotation_q = -sin_table[dithered_phase];

    phase += frequency;
    //const int16_t i_shifted = (((i * rotation_i) - (q * rotation_q))+k) >> 15;
    //const int16_t q_shifted = (((q * rotation_i) + (i * rotation_q))+k) >> 15;
    const int16_t i_shifted = (((int32_t)i * rotation_i) - ((int32_t)q * rotation_q)) >> 15;
    const int16_t q_shifted = (((int32_t)q * rotation_i) + ((int32_t)i * rotation_q)) >> 15;

    i = i_shifted;
    q = q_shifted;
}

bool __not_in_flash_func(rx_dsp :: decimate)(int16_t &i, int16_t &q)
//bool rx_dsp :: decimate(int16_t &i, int16_t &q)
{
      //CIC decimation filter
      //implement integrator stages
      integratori1 += i;
      integratorq1 += q;
      integratori2 += integratori1;
      integratorq2 += integratorq1;
      integratori3 += integratori2;
      integratorq3 += integratorq2;
      integratori4 += integratori3;
      integratorq4 += integratorq3;

      decimate_count++;
      if(decimate_count == decimation_rate)
      {
        decimate_count = 0;

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
        int16_t decimated_i = combi4>>(bit_growth-extra_bits);
        int16_t decimated_q = combq4>>(bit_growth-extra_bits);

        //first half band decimating filter
        bool new_sample = half_band_filter_inst.filter(decimated_i, decimated_q);

        //second half band filter (not decimating)
        if(new_sample)
        {
           half_band_filter2_inst.filter(decimated_i, decimated_q);
           i = decimated_i;
           q = decimated_q;
           return true;
        }
      }

      return false;
}

int16_t rx_dsp :: demodulate(int16_t i, int16_t q)
{
    if(mode == AM)
    {
        int16_t amplitude = rectangular_2_magnitude(i, q);
        //measure DC using first order IIR low-pass filter
        audio_dc = amplitude+(audio_dc - (audio_dc >> 5));
        //subtract DC component
        return amplitude - (audio_dc >> 5);
    }
    else if(mode == FM | mode == WFM)
    {
        const int16_t i_freq = (((int32_t)i * last_i) - ((int32_t)q * last_q));
        const int16_t q_freq = (((int32_t)q * last_i) + ((int32_t)i * last_q));
        int16_t frequency = rectangular_2_phase(i_freq, q_freq);
        printf("%i %i %i %i %i %i %i\n", i, q, last_i, last_q, i_freq, q_freq, frequency);
        last_i = i;
        last_q = -q;
        return frequency;
    }
    else if(mode == LSB || mode == USB)
    {
        //shift frequency by +FS/4
        //      __|__
        //  ___/  |  \___
        //        |
        //  <-----+----->

        //        | ____
        //  ______|/    \
        //        |
        //  <-----+----->

        //filter -Fs/4 to +Fs/4

        //        | __  
        //  ______|/  \__
        //        |
        //  <-----+----->


        if(mode == USB)
        {
          ssb_phase = (ssb_phase + 1) & 3u;
        }
        else
        {
          ssb_phase = (ssb_phase - 1) & 3u;
        }

        const int16_t sample_i[4] = {i, q, -i, -q};
        const int16_t sample_q[4] = {q, -i, -q, i};
        int16_t ii = sample_i[ssb_phase];
        int16_t qq = sample_q[ssb_phase];
        ssb_filter.filter(ii,  qq);

        //shift frequency by -FS/4 and discard q to form a real (not complex) sample
        //        | __  
        //  ______|/  \__
        //        |
        //  <-----+----->

        //     __ |     
        //  __/  \|______
        //        |
        //  <-----+----->

        //     __ | __   
        //  __/  \|/  \__
        //        |
        //  <-----+----->

        const int16_t audio[4] = {-qq, -ii, qq, ii};
        return audio[ssb_phase];

    }
    else //if(mode==cw)
    {
      int16_t ii = i;
      int16_t qq = q;
      if(cw_decimate(ii, qq)){
        cw_i = ii;
        cw_q = qq;
      }
      cw_sidetone_phase += cw_sidetone_frequency_Hz * 2048 * decimation_rate * 2 / adc_sample_rate;
      const int16_t rotation_i =  sin_table[(cw_sidetone_phase + 512u) & 0x7ffu];
      const int16_t rotation_q = -sin_table[cw_sidetone_phase & 0x7ffu];
      return ((cw_i * rotation_i) - (cw_q * rotation_q)) >> 15;
    }
}

bool rx_dsp :: cw_decimate(int16_t &i, int16_t &q)
{
      //CIC decimation filter
      //implement integrator stages
      cw_integratori1 += i;
      cw_integratorq1 += q;
      cw_integratori2 += cw_integratori1;
      cw_integratorq2 += cw_integratorq1;
      cw_integratori3 += cw_integratori2;
      cw_integratorq3 += cw_integratorq2;
      cw_integratori4 += cw_integratori3;
      cw_integratorq4 += cw_integratorq3;

      cw_decimate_count++;
      if(cw_decimate_count == cw_decimation_rate)
      {
        cw_decimate_count = 0;

        //implement comb stages
        const int32_t cw_combi1 = cw_integratori4-cw_delayi0;
        const int32_t cw_combq1 = cw_integratorq4-cw_delayq0;
        const int32_t cw_combi2 = cw_combi1-cw_delayi1;
        const int32_t cw_combq2 = cw_combq1-cw_delayq1;
        const int32_t cw_combi3 = cw_combi2-cw_delayi2;
        const int32_t cw_combq3 = cw_combq2-cw_delayq2;
        const int32_t cw_combi4 = cw_combi3-cw_delayi3;
        const int32_t cw_combq4 = cw_combq3-cw_delayq3;
        cw_delayi0 = cw_integratori4;
        cw_delayq0 = cw_integratorq4;
        cw_delayi1 = cw_combi1;
        cw_delayq1 = cw_combq1;
        cw_delayi2 = cw_combi2;
        cw_delayq2 = cw_combq2;
        cw_delayi3 = cw_combi3;
        cw_delayq3 = cw_combq3;
        int16_t decimated_i = cw_combi4>>cw_bit_growth;
        int16_t decimated_q = cw_combq4>>cw_bit_growth;

        //first half band decimating filter
        bool new_sample = cw_half_band_filter_inst.filter(decimated_i, decimated_q);

        //second half band filter (not decimating)
        if(new_sample)
        {
           cw_half_band_filter2_inst.filter(decimated_i, decimated_q);
           i = decimated_i;
           q = decimated_q;
           return true;
        }
      }

      return false;
}

int16_t rx_dsp::automatic_gain_control(int16_t audio_in)
{
    //Use a leaky max hold to estimate audio power
    //             _
    //            | |
    //            | |
    //    audio __| |_____________________
    //            | |
    //            |_|
    //
    //                _____________
    //               /             \_
    //    max_hold  /                \_
    //           _ /                   \_
    //              ^                ^
    //            attack             |
    //                <---hang--->   |
    //                             decay

    // Attack is fast so that AGC reacts fast to increases in power
    // Hang time and decay are relatively slow to prevent rapid gain changes

    static const uint8_t extra_bits = 16;
    int32_t audio = audio_in;
    const int32_t audio_scaled = audio << extra_bits;
    if(audio_scaled > max_hold)
    {
      //attack
      max_hold += (audio_scaled - max_hold) >> attack_factor;
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

    //calculate gain needed to amplify to full scale
    const int16_t magnitude = max_hold >> extra_bits;
    const int16_t limit = INT16_MAX; //hard limit
    const int16_t setpoint = limit/2; //about half full scale

    //apply gain
    if(magnitude > 0)
    {
      int16_t gain = setpoint/magnitude;
      if(gain < 1) gain = 1;
      audio *= gain;
    }

    //soft clip (compress)
    if (audio > setpoint)  audio =  setpoint + ((audio-setpoint)>>1);
    if (audio < -setpoint) audio = -setpoint - ((audio+setpoint)>>1);

    //hard clamp
    if (audio > limit)  audio = limit;
    if (audio < -limit) audio = -limit;

    return audio;
}

rx_dsp :: rx_dsp()
{

  //initialise state
  dc = 0;
  phase = 0;
  decimate_count=0;
  frequency=0;
  initialise_luts();

  //initialise semaphore for spectrum
  sem_init(&spectrum_semaphore, 1, 1);

  //clear cic filter
  set_decimation_rate(20u);
  integratori1=0; integratorq1=0;
  integratori2=0; integratorq2=0;
  integratori3=0; integratorq3=0;
  integratori4=0; integratorq4=0;
  delayi0=0; delayq0=0;
  delayi1=0; delayq1=0;
  delayi2=0; delayq2=0;
  delayi3=0; delayq3=0;

  //clear cw filter
  cw_integratori1=0; cw_integratorq1=0;
  cw_integratori2=0; cw_integratorq2=0;
  cw_integratori3=0; cw_integratorq3=0;
  cw_integratori4=0; cw_integratorq4=0;
  cw_delayi0=0; cw_delayq0=0;
  cw_delayi1=0; cw_delayq1=0;
  cw_delayi2=0; cw_delayq2=0;
  cw_delayi3=0; cw_delayq3=0;

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
  offset_frequency_Hz = offset_frequency;
  frequency = ((double)(1ull<<32)*offset_frequency)/adc_sample_rate;
}

void rx_dsp :: set_mode(uint8_t val)
{
  mode = val;
  if(mode == LSB || mode == USB)
  {
    //250e3/(25*2*2) == 2.5kHz
    set_decimation_rate(25u);
  }
  if(mode == WFM)
  {
    //250e3/1 == 250kHz
    set_decimation_rate(1u);
  }
  if(mode == FM)
  {
    //250e3/(14*2) == 9kHz
    set_decimation_rate(14u);
  }
  else
  {
    //250e3/(20*2) == 6kHz
    set_decimation_rate(20u);
  }
}

void rx_dsp :: set_cw_sidetone_Hz(uint16_t val)
{
  cw_sidetone_frequency_Hz = val;
}

//volume settings 0 to 9
void rx_dsp :: set_volume(uint8_t val)
{
  const int16_t gain[] = {
    0,   // 0 = 0/256 -infdB
    16,  // 1 = 16/256 -24dB
    23,  // 2 = 23/256 -21dB
    32,  // 3 = 32/256 -18dB
    45,  // 4 = 45/256 -15dB
    64,  // 5 = 64/256 -12dB
    90,  // 6 = 90/256  -9dB
    128, // 7 = 128/256 -6dB
    180, // 8 = 180/256 -3dB
    256  // 9 = 256/256  0dB
  };
  gain_numerator = gain[val];
}

//set_squelch
void rx_dsp :: set_squelch(uint8_t val)
{
  //0-9 = s0 to s9, 10 to 12 = S9+10dB to S9+30dB
  const int16_t thresholds[] = {
    s9_threshold>>9, //s0
    s9_threshold>>8, //s1
    s9_threshold>>7, //s2
    s9_threshold>>6, //s3
    s9_threshold>>5, //s4
    s9_threshold>>4, //s5
    s9_threshold>>3, //s6
    s9_threshold>>2, //s7
    s9_threshold>>1, //s8
    s9_threshold,    //s9
    s9_threshold*3,  //s9+10dB
    s9_threshold*10, //s9+20dB
    s9_threshold*31, //s9+30dB
  };
  squelch_threshold = thresholds[val];
}

int16_t rx_dsp :: get_signal_strength_dBm()
{
  const float signal_strength_dBFS = 20.0*log10f((float)signal_amplitude / full_scale_signal_strength);
  return roundf(full_scale_dBm - amplifier_gain_dB + signal_strength_dBFS);
}

void rx_dsp :: set_decimation_rate(uint8_t i) 
{
  decimation_rate = i;
  interpolation_rate = i;
  bit_growth = ceilf(log2f(decimation_rate))*4;
  const float growth_adjustment = decimation_rate/powf(2, ceil(log2f(decimation_rate))); //growth in decimating filters
  full_scale_signal_strength = 0.707f*adc_max*(1<<extra_bits)*growth_adjustment; //signal strength after decimator expected for a full scale sin wave
  s9_threshold = full_scale_signal_strength*powf(10.0f, (S9 - full_scale_dBm + amplifier_gain_dB)/20.0f);
}

void rx_dsp :: get_spectrum(float spectrum[], int16_t &offset)
{
  static float flt_capture_i[4][256];
  static float flt_capture_q[4][256];

  //convert capture to frequency domain
  sem_acquire_blocking(&spectrum_semaphore);

  //convert to float
  uint16_t k = 0;
  for(uint16_t i=0; i<256; i++)
  {
    for(uint16_t j=0; j<4; j++)
    {
      float window = 0.54f - 0.46f*cosf(2.0f*M_PI*i/256.0f);
      flt_capture_i[j][i] = (float)capture_i[k]*window;
      flt_capture_q[j][i] = (float)capture_q[k++]*window;
    }
  }

  sem_release(&spectrum_semaphore);

  //FFT and magnitude
  static float segments[4][256];
  for(uint16_t j=0; j<4; j++)
  {
    fft(flt_capture_i[j], flt_capture_q[j]);
    uint16_t f=0;
    for(uint16_t i=192; i<256; i++) segments[j][f++] = flt_rectangular_2_magnitude(flt_capture_i[j][i], flt_capture_q[j][i]);
    for(uint16_t i=0; i<64; i++) segments[j][f++] = flt_rectangular_2_magnitude(flt_capture_i[j][i], flt_capture_q[j][i]);
  }

  for(uint16_t i=0; i<256; i++)
  {
    float average = 0.0f;
    for(uint16_t j=0; j<4; j++) average += segments[j][i];
    spectrum[i] = average;
  }


  offset = 62 + ((offset_frequency_Hz*256)/(int32_t)adc_sample_rate);
}
