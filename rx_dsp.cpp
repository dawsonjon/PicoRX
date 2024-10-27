#include "rx_dsp.h"
#include "rx_definitions.h"
#include "fft_filter.h"
#include "utils.h"
#include "pico/stdlib.h"
#include "cic_corrections.h"

#include <math.h>
#include <cstdio>
#include <algorithm>

static const int16_t deemph_taps[2][3] = {{14430, 14430, -3909}, {10571, 10571, -11626}};
int16_t __not_in_flash_func(rx_dsp :: apply_deemphasis)(int16_t x)
{
  if(deemphasis == 0)
    return x;

  static int16_t x1 = 0;
  static int16_t y1 = 0;

  size_t i = deemphasis - 1;

  int16_t y = ((x * deemph_taps[i][0]) >> 15) + ((x1 * deemph_taps[i][1]) >> 15) - ((y1 * deemph_taps[0][2]) >> 15);
  x1 = x;
  y1 = y;
  return y;
}

static uint32_t __not_in_flash_func(intsqrt)(const uint32_t n) {
    uint8_t shift = 32u;
    shift += shift & 1; // round up to next multiple of 2

    uint32_t result = 0;

    do {
        shift -= 2;
        result <<= 1; // leftshift the result to make the next guess
        result |= 1;  // guess that the next bit is 1
        result ^= result * result > (n >> shift); // revert if guess too high
    } while (shift != 0);

    return result;
}

void inline rx_dsp :: iq_imbalance_correction(int16_t &i, int16_t &q)
{
    if (iq_correction)
    {
      static uint16_t index = 0;
      static int32_t theta1 = 0;
      static int32_t theta2 = 0;
      static int32_t theta3 = 0;

      theta1 += ((i < 0) ? -q : q);
      theta2 += ((i < 0) ? -i : i);
      theta3 += ((q < 0) ? -q : q);

      static int32_t c1 = 0;
      static int32_t c2 = 0;
      if (++index == 512)
      {             
        static int64_t theta1_filtered = 0;
        static int64_t theta2_filtered = 0;
        static int64_t theta3_filtered = 0;
        theta1_filtered = theta1_filtered - (theta1_filtered >> 5) + (-theta1 >> 5);
        theta2_filtered = theta2_filtered - (theta2_filtered >> 5) + (theta2 >> 5);
        theta3_filtered = theta3_filtered - (theta3_filtered >> 5) + (theta3 >> 5);

        //try to constrain square to less than 32 bits.
        //Assue that i/q used full int16_t range.
        //Accumulating 512 samples adds 9 bits of growth, so remove 18 after square.
        const int64_t theta1_squared = (theta1_filtered * theta1_filtered) >> 18; 
        const int64_t theta2_squared = (theta2_filtered * theta2_filtered) >> 18;
        const int64_t theta3_squared = (theta3_filtered * theta3_filtered) >> 18;

        c1 = (theta1_filtered << 15)/theta2_filtered;
        c2 = intsqrt(((theta3_squared - theta1_squared) << 30)/theta2_squared);

        theta1 = 0;
        theta2 = 0;
        theta3 = 0;
        index = 0;
      }

      q += ((int32_t)i * c1) >> 15;
      i = ((int32_t)i * c2) >> 15;
    }
}

uint16_t __not_in_flash_func(rx_dsp :: process_block)(uint16_t samples[], int16_t audio_samples[])
{

  uint16_t decimated_index = 0;
  int32_t magnitude_sum = 0;
  int16_t real[adc_block_size/cic_decimation_rate];
  int16_t imag[adc_block_size/cic_decimation_rate];

  for(uint16_t idx=0; idx<adc_block_size; idx++)
  {
      //convert to signed representation
      const int16_t raw_sample = samples[idx];

      //work out which samples are i and q
      int16_t i = ((idx&1)^1^swap_iq)*raw_sample;//even samples contain i data
      int16_t q = ((idx&1)^swap_iq)*raw_sample;//odd samples contain q data

      //reduce sample rate by a factor of 16
      if(decimate(i, q))
      {

        static uint32_t iq_count = 0;
        static int32_t i_accumulator = 0;
        static int32_t q_accumulator = 0;
        static int16_t i_avg = 0;
        static int16_t q_avg = 0;
        i_accumulator += i;
        q_accumulator += q;
        if (++iq_count == 2048) //power of 2 avoids division
        {
          i_avg = i_accumulator / 2048;
          q_avg = q_accumulator / 2048;
          i_accumulator = 0;
          q_accumulator = 0;
          iq_count = 0;
        }
        i -= i_avg;
        q -= q_avg;

        iq_imbalance_correction(i, q);

        //Apply frequency shift (move tuned frequency to DC)
        frequency_shift(i, q);

        #ifdef MEASURE_DC_BIAS 
        static int64_t bias_measurement = 0; 
        static int32_t num_bias_measurements = 0; 
        if(num_bias_measurements == 100000) { 
          printf("DC BIAS x 100 %lli\n", bias_measurement/1000); 
          num_bias_measurements = 0; 
          bias_measurement = 0; 
        } 
        else { 
          num_bias_measurements++; 
          bias_measurement += i; 
        } 
        #endif 

        real[decimated_index] = i;
        imag[decimated_index] = q;
        ++decimated_index;
      }
  }

  //fft filter decimates a further 2x
  //if the capture buffer isn't in use, fill it
  filter_control.capture = sem_try_acquire(&spectrum_semaphore);
  capture_filter_control = filter_control;
  fft_filter_inst.process_sample(real, imag, filter_control, capture);
  if(filter_control.capture) sem_release(&spectrum_semaphore);

  for(uint16_t idx=0; idx<adc_block_size/decimation_rate; idx++)
  {
    int16_t i = real[idx];
    int16_t q = imag[idx];

    //Measure amplitude (for signal strength indicator)
    int32_t amplitude = rectangular_2_magnitude(i, q);
    magnitude_sum += amplitude;

    //Demodulate to give audio sample
    int32_t audio = demodulate(i, q);

    //De-emphasis
    audio = apply_deemphasis(audio);

    //Automatic gain control scales signal to use full 16 bit range
    //e.g. -32767 to 32767
    audio = automatic_gain_control(audio);

    //squelch
    if(signal_amplitude < squelch_threshold) {
      audio = 0;
    }

    //output raw audio
    audio_samples[idx] = audio;
  }

  //average over the number of samples
  signal_amplitude = (magnitude_sum * decimation_rate)/adc_block_size;

  return adc_block_size/decimation_rate;
}

void __not_in_flash_func(rx_dsp :: frequency_shift)(int16_t &i, int16_t &q)
{
    //Apply frequency shift (move tuned frequency to DC)         
    const uint16_t scaled_phase = (phase >> 21);
    const int16_t rotation_i =  sin_table[(scaled_phase+512u) & 0x7ff]; //32 - 21 = 11MSBs
    const int16_t rotation_q = -sin_table[scaled_phase];

    phase += frequency;
    //truncating fractional bits introduces bias, but it is more efficient to remove it after decimation
    int32_t bias = (1<<14);
    const int16_t i_shifted = (((int32_t)i * rotation_i) - ((int32_t)q * rotation_q) + bias) >> 15;
    const int16_t q_shifted = (((int32_t)q * rotation_i) + ((int32_t)i * rotation_q) + bias) >> 15;

    i = i_shifted;
    q = q_shifted;
}

bool __not_in_flash_func(rx_dsp :: decimate)(int16_t &i, int16_t &q)
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
      if(decimate_count >= cic_decimation_rate)
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

        //remove bit growth, but keep some extra bits since noise floor is now lower
        i = combi4>>(cic_bit_growth-extra_bits);
        q = combq4>>(cic_bit_growth-extra_bits);

        return true;
      }

      return false;
}

#define AMSYNC_ALPHA (3398)
#define AMSYNC_BETA (1898)
#define AMSYNC_F_MIN (-218)
#define AMSYNC_F_MAX (218)
#define AMSYNC_FIX_MAX (32767)

int16_t __not_in_flash_func(rx_dsp :: demodulate)(int16_t i, int16_t q)
{
   static int32_t phi_locked = 0;
   static int32_t freq_locked = 0;

    if(mode == AM)
    {
        int16_t amplitude = rectangular_2_magnitude(i, q);
        //measure DC using first order IIR low-pass filter
        audio_dc = amplitude+(audio_dc - (audio_dc >> 5));
        //subtract DC component
        return amplitude - (audio_dc >> 5);
    }
    else if(mode == AMSYNC)
    {
      size_t idx;

      if (phi_locked < 0)
      {
        idx = AMSYNC_FIX_MAX + 1 + phi_locked;
      }
      else
      {
        idx = phi_locked;
      }

      // VCO
      const int16_t vco_i = sin_table[((idx >> 4) + 512u) & 0x7ffu];
      const int16_t vco_q = sin_table[(idx >> 4) & 0x7ffu];

      // Phase Detector
      const int16_t synced_i = (i * vco_i + q * vco_q) >> 15;
      const int16_t synced_q = (-i * vco_q + q * vco_i) >> 15;
      int16_t err = -rectangular_2_phase(synced_i, synced_q);

      // Loop filter
      freq_locked += (((int32_t)AMSYNC_BETA * err) >> 15);
      phi_locked += freq_locked + (((int32_t)AMSYNC_ALPHA * err) >> 15);

      // Clamp frequency
      if (freq_locked > AMSYNC_F_MAX)
      {
        freq_locked = AMSYNC_F_MAX;
      }

      if (freq_locked < AMSYNC_F_MIN)
      {
        freq_locked = AMSYNC_F_MIN;
      }

      // Wrap phi
      if (phi_locked > AMSYNC_FIX_MAX)
      {
        phi_locked -= AMSYNC_FIX_MAX + 1;
      }

      if (phi_locked < -AMSYNC_FIX_MAX)
      {
        phi_locked += AMSYNC_FIX_MAX + 1;
      }

      // measure DC using first order IIR low-pass filter
      audio_dc = synced_q + (audio_dc - (audio_dc >> 5));
      // subtract DC component
      return synced_q - (audio_dc >> 5);
    }
    else if(mode == FM)
    {
        int16_t phase = rectangular_2_phase(i, q);
        int16_t frequency = phase - last_phase;
        last_phase = phase;

        return frequency;
    }
    else if(mode == LSB || mode == USB)
    {
        return i;
    }
    else //if(mode==cw)
    {
      cw_sidetone_phase += cw_sidetone_frequency_Hz * 2048 * decimation_rate / adc_sample_rate;
      const int16_t rotation_i =  sin_table[(cw_sidetone_phase + 512u) & 0x7ffu];
      const int16_t rotation_q = -sin_table[cw_sidetone_phase & 0x7ffu];
      return ((i * rotation_i) - (q * rotation_q)) >> 15;
    }
}

int16_t __not_in_flash_func(rx_dsp::automatic_gain_control)(int16_t audio_in)
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
      if(manual_gain_control)
      {
        gain = manual_gain;
      }
      else
      {
        gain = setpoint/magnitude;
      }
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
  phase = 0;
  frequency=0;
  initialise_luts();
  swap_iq = 0;
  iq_correction = 0;

  //initialise semaphore for spectrum
  set_mode(AM, 2);
  sem_init(&spectrum_semaphore, 1, 1);
  set_agc_speed(3);
  filter_control.enable_auto_notch = false;

  //clear cic filter
  decimate_count=0;
  integratori1=0; integratorq1=0;
  integratori2=0; integratorq2=0;
  integratori3=0; integratorq3=0;
  integratori4=0; integratorq4=0;
  delayi0=0; delayq0=0;
  delayi1=0; delayq1=0;
  delayi2=0; delayq2=0;
  delayi3=0; delayq3=0;
}

void rx_dsp :: set_auto_notch(bool enable_auto_notch)
{
  filter_control.enable_auto_notch = enable_auto_notch;
}

void rx_dsp :: set_deemphasis(uint8_t deemph)
{
  deemphasis = deemph;
}

void rx_dsp :: set_agc_speed(uint8_t agc_setting)
{
  //input fs=480000.000000 Hz
  //decimation=32 x 2
  //fs=15625.000000 Hz
  //Setting Decay Time(s) Factor Attack Time(s) Factor  Hang  Timer
  //======= ============= ====== ============== ======  ====  =====
  //fast        0.151          10       0.001      2    0.1s   1500
  //medium      0.302          11       0.001      2    0.25s  3750
  //slow        0.604          12       0.001      2    1s     15000
  //long        2.414          14       0.001      2    2s     30000


  manual_gain_control = false;
  manual_gain = 1;

  switch(agc_setting)
  {
      case 0: //fast
        attack_factor=2;
        decay_factor=10;
        hang_time=1500;
        break;

      case 1: //medium
        attack_factor=2;
        decay_factor=11;
        hang_time=3750;
        break;

      case 2: //slow
        attack_factor=2;
        decay_factor=12;
        hang_time=15000;
        break;

      case 3: //long
        attack_factor=2;
        decay_factor=14;
        hang_time=30000;
        break;

      default://4=6dB-60dB,
        manual_gain = 1 << (agc_setting - 4);
        manual_gain_control = true;
        break;
  }
}

void rx_dsp :: set_frequency_offset_Hz(double offset_frequency)
{
  offset_frequency_Hz = offset_frequency;
  const float bin_width = adc_sample_rate/(cic_decimation_rate*256);
  filter_control.fft_bin = offset_frequency/bin_width;
  frequency = ((double)(1ull<<32)*offset_frequency)*cic_decimation_rate/(adc_sample_rate);
}


void rx_dsp :: set_mode(uint8_t val, uint8_t bw)
{
  mode = val;
  //                           AM AMS LSB USB NFM CW
  uint8_t start_bins[6]   =  {  0,  0,  3,  3,  0, 0};

  uint8_t stop_bins[5][6] = {{ 19, 19, 16, 16, 31, 0},  //very narrow
                             { 22, 22, 19, 19, 34, 1},  //narrow
                             { 25, 25, 22, 22, 37, 2},  //normal
                             { 28, 28, 25, 25, 40, 3},  //wide
                             { 31, 31, 28, 28, 43, 4}}; //very wide

  filter_control.lower_sideband = (mode != USB);
  filter_control.upper_sideband = (mode != LSB);
  filter_control.start_bin = start_bins[mode];
  filter_control.stop_bin = stop_bins[bw][mode];
}

void rx_dsp :: set_swap_iq(uint8_t val)
{
  swap_iq = val;
}

void rx_dsp :: set_iq_correction(uint8_t val)
{
  iq_correction = val;
}

void rx_dsp :: set_cw_sidetone_Hz(uint16_t val)
{
  cw_sidetone_frequency_Hz = val;
}

void rx_dsp :: set_gain_cal_dB(uint16_t val)
{
  amplifier_gain_dB = val;
  s9_threshold = full_scale_signal_strength*powf(10.0f, (S9 - full_scale_dBm + amplifier_gain_dB)/20.0f);
}

//set_squelch
void rx_dsp :: set_squelch(uint8_t val)
{
  //0-9 = s0 to s9, 10 to 12 = S9+10dB to S9+30dB
  const int16_t thresholds[] = {
    (int16_t)(s9_threshold>>9), //s0
    (int16_t)(s9_threshold>>8), //s1
    (int16_t)(s9_threshold>>7), //s2
    (int16_t)(s9_threshold>>6), //s3
    (int16_t)(s9_threshold>>5), //s4
    (int16_t)(s9_threshold>>4), //s5
    (int16_t)(s9_threshold>>3), //s6
    (int16_t)(s9_threshold>>2), //s7
    (int16_t)(s9_threshold>>1), //s8
    (int16_t)(s9_threshold),    //s9
    (int16_t)(s9_threshold*3),  //s9+10dB
    (int16_t)(s9_threshold*10), //s9+20dB
    (int16_t)(s9_threshold*31), //s9+30dB
  };
  squelch_threshold = thresholds[val];
}

int16_t rx_dsp :: get_signal_strength_dBm()
{
  if(signal_amplitude == 0)
  {
    return -130;
  }
  const float signal_strength_dBFS = 20.0*log10f((float)signal_amplitude / full_scale_signal_strength);
  return roundf(full_scale_dBm - amplifier_gain_dB + signal_strength_dBFS);
}

s_filter_control rx_dsp :: get_filter_config()
{
  return capture_filter_control;
}

static int16_t cic_correct(int16_t fft_bin, int16_t fft_offset, uint16_t magnitude)
{
  int16_t corrected_fft_bin = (fft_bin + fft_offset);
  if(corrected_fft_bin > 127) corrected_fft_bin -= 256;
  if(corrected_fft_bin < -128) corrected_fft_bin += 256;
  uint16_t unsigned_fft_bin = abs(corrected_fft_bin); 
  uint32_t adjusted_magnitude = ((uint32_t)magnitude * cic_correction[unsigned_fft_bin]) >> 8;
  return std::min(adjusted_magnitude, (uint32_t)UINT16_MAX);
}

static inline int8_t freq_bin(uint8_t bin)
{
  return bin > 127 ? bin - 256 : bin;
}

static inline uint8_t fft_shift(uint8_t bin)
{
  return bin ^ 0x80;
}

void rx_dsp :: get_spectrum(uint8_t spectrum[], uint8_t &dB10)
{
  //FFT and magnitude
  sem_acquire_blocking(&spectrum_semaphore);

  //find minimum and maximum values
  const uint16_t lowest_max = 2500u;
  static uint16_t max=65523u;//long term maximum
  uint16_t new_max=0u;
  static uint16_t min=1u;//long term maximum
  uint16_t new_min=65535u;
  for(uint16_t i=0; i<256; ++i)
  {
    const uint16_t magnitude = cic_correct(freq_bin(i), capture_filter_control.fft_bin, capture[i]);
    if(magnitude == 0) continue;
    new_max = std::max(magnitude, new_max);
    new_min = std::min(magnitude, new_min);
  }
  max=max - (max >> 1) + (new_max >> 1);
  min=min - (min >> 1) + (new_min >> 1);
  const float logmin = log10f(min);
  const float logmax = log10f(std::max(max, lowest_max));

  //clamp and convert to log scale 0 -> 255
  for(uint16_t i=0; i<256; i++)
  {
    const uint16_t magnitude = cic_correct(freq_bin(i), capture_filter_control.fft_bin, capture[i]);
    if(magnitude == 0)
    {
      spectrum[fft_shift(i)] = 0u;
    } else {
      const float normalised = 255.0f*(log10f(magnitude)-logmin)/(logmax-logmin);
      const float clamped = std::max(std::min(normalised, 255.0f), 0.0f);
      spectrum[fft_shift(i)] = clamped;
    }
  }

  sem_release(&spectrum_semaphore);

  //number steps representing 10dB
  dB10 = 256/(2*logf(max/min));
}
