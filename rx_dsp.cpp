#include "rx_dsp.h"
#include "rx_definitions.h"

#include "utils.h"
#include <math.h>
#include <cstdio>

#include "ring_buffer_lib.h"

#include "pico/stdlib.h"
#include "usb_audio_device.h"

static const int16_t deemph_taps[2][3] = {{14430, 14430, -3909}, {10571, 10571, -11626}};

static inline void rolling_avg_int(int16_t nv, int32_t *avg, int16_t *err)
{
  const int16_t t = 10;
  const int16_t d = nv - *avg;

  *avg += d / t;
  *err += d % t;

  if (abs(*err) >= t)
  {
    *avg += *err / t;
    *err %= t;
  }
}

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

static void __not_in_flash_func(interp_bresenham)(int16_t y1, int16_t y2, uint16_t nx, int16_t *ny)
{
  const int16_t x1 = 0;
  const int16_t x2 = nx - 1;
  const int16_t dx = x2 - x1;

  int16_t d, dy, ai, bi, yi;
  int16_t x = x1;
  int16_t y = y1;

  if (y1 < y2)
  {
    yi = 1;
    dy = y2 - y1;
  }
  else
  {
    yi = -1;
    dy = y1 - y2;
  }

  ny[x] = y;

  if (dx > dy)
  {
    ai = (dy - dx) * 2;
    bi = dy * 2;
    d = bi - dx;

    while (x != x2)
    {
      if (d >= 0)
      {
        x++;
        y += yi;
        d += ai;
      }
      else
      {
        d += bi;
        x++;
      }
      ny[x] = y;
    }
  }
  else
  {
    ai = (dx - dy) * 2;
    bi = dx * 2;
    d = bi - dy;

    while (y != y2)
    {
      if (d >= 0)
      {
        x++;
        y += yi;
        d += ai;
      }
      else
      {
        d += bi;
        y += yi;
      }
      ny[x] = y;
    }
  }
}

#define USB_BUF_SIZE (sizeof(int16_t) * 2 * (adc_block_size/decimation_rate))
static ring_buffer_t usb_rb;
static uint8_t usb_buf[USB_BUF_SIZE];

critical_section_t usb_volumute;
static int16_t usb_volume=180;  // usb volume
static bool usb_mute = false;     // usb mute control

uint16_t __not_in_flash_func(rx_dsp :: process_block)(uint16_t samples[], int16_t audio_samples[])
{

  uint16_t audio_index = 0;
  uint16_t decimated_index = 0;
  int32_t magnitude_sum = 0;
  int32_t sample_accumulator = 0;
  static int16_t prev_audio = 0;
  static int16_t usb_lev_err = 0;

  static uint32_t iq_count = 0;
  static int32_t i_accumulator = 0;
  static int32_t q_accumulator = 0;
  static int16_t i_dc = 0;
  static int16_t q_dc = 0;

  int16_t real[adc_block_size/cic_decimation_rate];
  int16_t imag[adc_block_size/cic_decimation_rate];

  for(uint16_t idx=0; idx<adc_block_size; idx++)
  {
      //convert to signed representation
      const int16_t raw_sample = samples[idx];
      sample_accumulator += raw_sample;

      //remove dc
      const int16_t sample = raw_sample - dc;

      //work out which samples are i and q
      int16_t i = ((idx&1)^1^swap_iq)*sample;//even samples contain i data
      int16_t q = ((idx&1)^swap_iq)*sample;//odd samples contain q data

      //reduce sample rate by a factor of 16
      if(decimate(i, q))
      {

        #ifdef HIGH_PASS_FILTERING
        static int16_t prev_i_in = 0;
        static int16_t prev_i_out = 0;
        static int16_t prev_q_in = 0;
        static int16_t prev_q_out = 0;

        int16_t input_i = i;
        int16_t input_q = q;

        int16_t bias = 1<<7;
        i = ((29774 * (int32_t)(prev_i_out + input_i - prev_i_in))+bias) >> 15;
        q = ((29774 * (int32_t)(prev_q_out + input_q - prev_q_in))+bias) >> 15;

        prev_i_in = input_i;
        prev_q_in = input_q;
        prev_i_out = i;
        prev_q_out = q;
        #endif 

        i_accumulator += i;
        q_accumulator += q;
        if(++iq_count == 1500)
        {
          i_dc = i_accumulator / 1500;
          q_dc = q_accumulator / 1500;
          i_accumulator = 0;
          q_accumulator = 0;
          iq_count = 0;
        }

        i -= i_dc;
        q -= q_dc;
        
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
  fft_filter_inst.process_sample(real, imag, filter_control, capture_i, capture_q);
  if(filter_control.capture) sem_release(&spectrum_semaphore);

  int16_t tmp_usb_buf[1 + (adc_block_size/decimation_rate)];
  critical_section_enter_blocking(&usb_volumute);
  int32_t safe_usb_volume = usb_volume;
  bool safe_usb_mute = usb_mute;
  critical_section_exit(&usb_volumute);

  for(uint16_t idx=0; idx<adc_block_size/decimation_rate; idx++)
  {
      int16_t i = real[idx];
      int16_t q = imag[idx];

      //Measure amplitude (for signal strength indicator)
      int32_t amplitude = rectangular_2_magnitude(i, q);
      magnitude_sum += amplitude;

      //Demodulate to give audio sample
      int32_t audio = demodulate(i, q);

      audio = apply_deemphasis(audio);

      //Automatic gain control scales signal to use full 16 bit range
      //e.g. -32767 to 32767
      int32_t usbaudio = audio = automatic_gain_control(audio);

      // usbaudio volume is controlled from usb so duplicate the sample
      if (safe_usb_mute) {
        usbaudio = 0;
      } else {
        usbaudio = (usbaudio * safe_usb_volume)/180;
      }

      //digital volume control
      audio = ((int32_t)audio * gain_numerator) >> 8;

      //squelch
      if(signal_amplitude < squelch_threshold) {
        usbaudio = audio = 0;
        squelch_state = false;
      } else {
        squelch_state = true;
      }

      tmp_usb_buf[idx] = usbaudio;

      //convert to unsigned value in range 0 to 500 to output to PWM
      audio += INT16_MAX;
      audio /= pwm_scale;

      interp_bresenham(prev_audio, audio, interpolation_rate, &audio_samples[audio_index]);
      audio_index += interpolation_rate;
      prev_audio = audio;
    }

    ring_buffer_push_ovr(&usb_rb, (uint8_t *)tmp_usb_buf, sizeof(int16_t) * (adc_block_size / decimation_rate));
    rolling_avg_int(ring_buffer_get_num_bytes(&usb_rb), &usb_buf_level_avg, &usb_lev_err);

    //average over the number of samples
    signal_amplitude = (magnitude_sum * decimation_rate)/adc_block_size;
    dc = sample_accumulator/adc_block_size;

    return audio_index;
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
        int32_t bias = 1 << (cic_bit_growth-extra_bits-1);
        i = (combi4-bias)>>(cic_bit_growth-extra_bits);
        q = (combq4-bias)>>(cic_bit_growth-extra_bits);

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

// usb mute setting = true is muted
static void on_usb_set_mutevol(bool mute, int16_t vol)
{
  //printf ("usbcb: got mute %d vol %d\n", mute, vol);
  critical_section_enter_blocking(&usb_volumute);
  usb_volume = vol + 90; // defined as -90 to 90 => 0 to 180
  usb_mute = mute;
  critical_section_exit(&usb_volumute);
}

static void __not_in_flash_func(on_usb_audio_tx_ready)()
{
  uint8_t usb_buf[SAMPLE_BUFFER_SIZE * sizeof(int16_t)] = {0};

  // Callback from TinyUSB library when all data is ready
  // to be transmitted.
  //
  // Write local buffer to the USB microphone
  uint16_t s = ring_buffer_pop(&usb_rb, usb_buf, sizeof(usb_buf));
  usb_audio_device_write(usb_buf, s);
}

rx_dsp :: rx_dsp()
{

  //initialise state
  dc = 0;
  phase = 0;
  frequency=0;
  initialise_luts();
  swap_iq = 0;

  //initialise semaphore for spectrum
  set_mode(AM, 2);
  sem_init(&spectrum_semaphore, 1, 1);
  ring_buffer_init(&usb_rb, usb_buf, USB_BUF_SIZE, 1);
  set_agc_speed(3);
  filter_control.enable_auto_notch = false;

  //initialise PWM frequency
  pwm_scale = 1+((INT16_MAX * 2)/500);

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

  for(uint16_t i=0; i<256; i++) accumulator[i] = 0.0f;
}

void rx_dsp :: set_usb_callbacks(void)
{
    critical_section_init(&usb_volumute);
    usb_audio_device_set_tx_ready_handler(on_usb_audio_tx_ready);
    usb_audio_device_set_mutevol_handler(on_usb_set_mutevol);
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

      default: //long
        attack_factor=2;
        decay_factor=14;
        hang_time=30000;
        break;
  }
}

void rx_dsp :: set_frequency_offset_Hz(double offset_frequency)
{
  offset_frequency_Hz = offset_frequency;
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

void rx_dsp :: set_cw_sidetone_Hz(uint16_t val)
{
  cw_sidetone_frequency_Hz = val;
}

void rx_dsp :: set_gain_cal_dB(uint16_t val)
{
  amplifier_gain_dB = val;
  s9_threshold = full_scale_signal_strength*powf(10.0f, (S9 - full_scale_dBm + amplifier_gain_dB)/20.0f);
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

void rx_dsp :: set_pwm_max(uint32_t pwm_max)
{
  pwm_scale = 1+((INT16_MAX * 2)/pwm_max);
}

bool rx_dsp :: get_squelch_state()
{
  return squelch_state;
}

uint8_t rx_dsp :: get_usb_buf_level(void)
{
  return 100 * usb_buf_level_avg / USB_BUF_SIZE;
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

void rx_dsp :: get_spectrum(float spectrum[])
{
  const float alpha = 0.1f;
  const float beta = 1.0f - alpha;

  //FFT and magnitude
  sem_acquire_blocking(&spectrum_semaphore);
  uint8_t f = 0;
  for(uint16_t i=128; i<256; i++)
  {
    accumulator[f] = (beta * accumulator[f]) + (alpha * rectangular_2_magnitude(capture_i[i], capture_q[i]));
    f++;
  }
  for(uint16_t i=0; i<127; i++)
  {
    accumulator[f] = (beta * accumulator[f]) + (alpha * rectangular_2_magnitude(capture_i[i], capture_q[i]));
    f++;
  }
  sem_release(&spectrum_semaphore);

  for(uint16_t i=0; i<128; i++) spectrum[i] = accumulator[i*2] + accumulator[(i*2)+1];
}
