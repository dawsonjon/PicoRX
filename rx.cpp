#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include <string.h>
#include <algorithm>

#include "rx.h"
#include "nco.h"
#include "fft_filter.h"
#include "utils.h"
#include "usb_audio_device.h"
#include "ring_buffer_lib.h"
#include "pins.h"
#include "pwm_audio_sink.h"
#include "clocks.h"
#include "transmit/adc.h"
#include "transmit/pwm.h"
#include "transmit/transmit_nco.h"
#include "transmit/modulator.h"
#include "transmit/cw_keyer.h"
//ring buffer for USB data
#define USB_BUF_SIZE (sizeof(int16_t) * 8 * (1 + (adc_block_size/decimation_rate)))
static ring_buffer_t usb_ring_buffer;
static uint8_t usb_buf[USB_BUF_SIZE];

//buffers and dma for ADC
int rx::adc_dma_ping;
int rx::adc_dma_pong;
dma_channel_config rx::ping_cfg;
dma_channel_config rx::pong_cfg;
uint16_t rx::ping_samples[adc_block_size];
uint16_t rx::pong_samples[adc_block_size];

bool rx::audio_running;

//dma for capture
int rx::capture_dma;
dma_channel_config rx::capture_cfg;

void rx::dma_handler() {


    // adc ping             ####    ####
    // adc pong                 ####    ####
    // processing ping          ### 
    // processing pong              ###

    if(dma_hw->ints0 & (1u << adc_dma_ping))
    {
      dma_channel_set_write_addr(adc_dma_ping, ping_samples, false);
      dma_hw->ints0 = 1u << adc_dma_ping;
    }

    if(dma_hw->ints0 & (1u << adc_dma_pong))
    {
      dma_channel_set_write_addr(adc_dma_pong, pong_samples, false);
      dma_hw->ints0 = 1u << adc_dma_pong;
    }

}


void rx::access(bool s)
{
  sem_acquire_blocking(&settings_semaphore);
  settings_changed |= s;
}

void rx::tune()
{
  //update the si5351 here rather than apply settings
  //since this function is called from core 0, this avoids the need
  //for additional synchronisation of i2c across cores

  sem_acquire_blocking(&settings_semaphore);
  if(settings_to_apply.enable_external_nco)
  {
    //disable internal nco
    if(internal_nco_active)
    {
      pio_sm_set_enabled(pio, sm, false);
      gpio_set_function(PIN_NCO_1, GPIO_FUNC_SIO);
      gpio_set_dir(PIN_NCO_1, GPIO_IN);
      gpio_set_function(PIN_NCO_2, GPIO_FUNC_SIO);
      gpio_set_dir(PIN_NCO_2, GPIO_IN);
      internal_nco_active = false;
      //use a fixed clock frequency when using external NCO
      uint32_t vco_freq = (12000000 / possible_frequencies[0].refdiv) * possible_frequencies[0].fbdiv;
      set_sys_clock_pll(vco_freq, possible_frequencies[0].postdiv1, possible_frequencies[0].postdiv2);
      system_clock_rate = possible_frequencies[0].frequency;
      pwm_audio_sink_update_pwm_max((system_clock_rate/pwm_audio_sample_rate)-1);
    }

    //initialise external nco before first use
    if(!external_nco_initialised)
    {
      external_nco_good = external_nco.initialise(OLED_I2C_INST, PIN_DISPLAY_SDA, PIN_DISPLAY_SCL, 0x60, 25000000);
      external_nco.set_drive(3);
      external_nco.crystal_load(3);
      external_nco.start();
      external_nco_initialised = true;
    }

    //start external oscillator each time it is enabled
    if(!external_nco_active)
    {
      external_nco.start();
      external_nco_active = true;
    }

    if(external_nco_good)
    {
      tuned_frequency_Hz = settings_to_apply.tuned_frequency_Hz;
      double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+settings_to_apply.ppm);
      if_mode = settings_to_apply.if_mode;
      if_frequency_hz_over_100 = settings_to_apply.if_frequency_hz_over_100;
      nco_frequency_Hz = external_nco.set_frequency_hz(adjusted_tuned_frequency_Hz + ((uint16_t)if_frequency_hz_over_100*100));
      offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);
    }
  }
  else
  {

    //disable external nco
    if(external_nco_initialised && external_nco_active)
    {
      external_nco.stop();
      external_nco_active = false;
    }

    //enable internal nco
    if(!internal_nco_active)
    {
      gpio_set_function(PIN_NCO_1, GPIO_FUNC_PIO0);
      gpio_set_dir(PIN_NCO_1, GPIO_OUT);
      gpio_set_function(PIN_NCO_2, GPIO_FUNC_PIO0);
      gpio_set_dir(PIN_NCO_2, GPIO_OUT);
      pio_sm_set_enabled(pio, sm, true);
      internal_nco_active = true;
    }

    if((tuned_frequency_Hz != settings_to_apply.tuned_frequency_Hz) || 
       (ppm != settings_to_apply.ppm) ||
       (if_mode != settings_to_apply.if_mode) ||
       (if_frequency_hz_over_100 != settings_to_apply.if_frequency_hz_over_100))
    {
      //apply frequency
      tuned_frequency_Hz = settings_to_apply.tuned_frequency_Hz;
      ppm = settings_to_apply.ppm;

      //apply frequency calibration
      double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+settings_to_apply.ppm);
      if_mode = settings_to_apply.if_mode;
      if_frequency_hz_over_100 = settings_to_apply.if_frequency_hz_over_100;
      
      //pwm_audio_sink_update_pwm_max(80);  // reduces audio clicks
      disable_pwm();
      nco_frequency_Hz = nco_set_frequency(pio, sm, adjusted_tuned_frequency_Hz, system_clock_rate, if_frequency_hz_over_100, if_mode);
      offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
      pwm_audio_sink_update_pwm_max((system_clock_rate/pwm_audio_sample_rate)-1);
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);

      sleep_us(15000);

      //apply pwm_max
      enable_pwm();
    }
  }

  sem_release(&settings_semaphore);
}


void rx::release()
{
  sem_release(&settings_semaphore);
}

void rx::update_status()
{

   const bool sem_acquired = sem_try_acquire(&settings_semaphore);
   if(sem_acquired)
   {
     suspend = settings_to_apply.suspend;

     //update status
     status.signal_strength_dBm = rx_dsp_inst.get_signal_strength_dBm();
     status.busy_time = busy_time;
     status.battery = battery;
     status.temp = temp;
     status.filter_config = rx_dsp_inst.get_filter_config();
     static uint16_t avg_level = 0;
     avg_level = (avg_level - (avg_level >> 2)) + (ring_buffer_get_num_bytes(&usb_ring_buffer) >> 2);
     status.usb_buf_level = 100 * avg_level / USB_BUF_SIZE;
     status.tuning_offset_Hz = rx_dsp_inst.get_tuning_offset_Hz();
     status.audio_level = tx_audio_level;
     status.transmitting = ptt();
     sem_release(&settings_semaphore);
   }
}

void rx::apply_settings()
{
   if(sem_try_acquire(&settings_semaphore))
   {

      if(tuned_frequency_Hz > (settings_to_apply.band_7_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 0);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_6_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 1);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_5_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 0);
        gpio_put(PIN_BAND_1, 1);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_4_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 1);
        gpio_put(PIN_BAND_1, 1);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_3_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 0);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 1);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_2_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 1);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 1);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_1_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 0);
        gpio_put(PIN_BAND_1, 1);
        gpio_put(PIN_BAND_2, 1);
      }
      else
      {
        gpio_put(PIN_BAND_0, 1);
        gpio_put(PIN_BAND_1, 1);
        gpio_put(PIN_BAND_2, 1);
      }


      //apply frequency offset
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);

      //apply CW sidetone
      rx_dsp_inst.set_cw_sidetone_Hz(settings_to_apply.cw_sidetone_Hz);

      //apply gain calibration
      rx_dsp_inst.set_gain_cal_dB(settings_to_apply.gain_cal);

      //apply AGC control
      rx_dsp_inst.set_agc_control(settings_to_apply.agc_setting, settings_to_apply.agc_gain);

      //apply Automatic Notch Filter
      rx_dsp_inst.set_auto_notch(settings_to_apply.enable_auto_notch);

      //apply Spectrum Smoothing
      rx_dsp_inst.set_spectrum_smoothing(settings_to_apply.spectrum_smoothing);

      //apply Noise Reduction
      rx_dsp_inst.set_noise_reduction(settings_to_apply.enable_noise_reduction, settings_to_apply.noise_estimation, settings_to_apply.noise_threshold);

      //apply mode
      rx_dsp_inst.set_mode(settings_to_apply.mode, settings_to_apply.bandwidth);


      //apply volume
      static const int16_t gain[] = {
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
      gain_numerator = gain[settings_to_apply.volume];
      //apply deemphasis
      rx_dsp_inst.set_deemphasis(settings_to_apply.deemphasis);

      //apply treble
      rx_dsp_inst.set_treble(settings_to_apply.treble);
    
      //apply bass
      rx_dsp_inst.set_bass(settings_to_apply.bass);

      //apply squelch
      rx_dsp_inst.set_squelch(settings_to_apply.squelch_threshold, settings_to_apply.squelch_timeout);

      //apply swap iq
      rx_dsp_inst.set_swap_iq(settings_to_apply.swap_iq);

      //apply iq imbalance correction
      rx_dsp_inst.set_iq_correction(settings_to_apply.iq_correction);

      stream_raw_iq = settings_to_apply.stream_raw_iq;
      transmit_mode = settings_to_apply.mode;
      tx_cw_paddle = settings_to_apply.cw_paddle;
      tx_cw_speed = settings_to_apply.cw_speed;
      tx_mic_gain = settings_to_apply.mic_gain;
      tx_modulation = settings_to_apply.tx_modulation;
      tx_pwm_min = settings_to_apply.pwm_min;
      tx_pwm_max = settings_to_apply.pwm_max;
      tx_pwm_threshold = settings_to_apply.pwm_threshold;

      settings_changed = false;
      sem_release(&settings_semaphore);
   }
}

void rx::get_spectrum(uint8_t spectrum[], uint8_t &dB10, uint8_t zoom)
{
  rx_dsp_inst.get_spectrum(spectrum, dB10, zoom);
}


rx::rx(rx_settings & settings_to_apply, rx_status & status) : dit(PIN_DIT), dah(PIN_DAH), settings_to_apply(settings_to_apply), status(status)
{

    settings_to_apply.suspend = false;
    suspend = false;
    stream_raw_iq = 0;

    //Configure PIO to act as quadrature oscilator
    pio = pio0;
    offset = pio_add_program(pio, &nco_program);
    sm = pio_claim_unused_sm(pio, true);
    nco_program_init(pio, sm, offset);

    system_clock_rate = possible_frequencies[0].frequency;
    pwm_audio_sink_update_pwm_max((system_clock_rate/pwm_audio_sample_rate)-1);

    ring_buffer_init(&usb_ring_buffer, usb_buf, USB_BUF_SIZE, 1);

    //configure SMPS into power save mode
    gpio_init(PIN_PSU);
    gpio_set_function(PIN_PSU, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_PSU, GPIO_OUT);
    gpio_put(PIN_PSU, 1);
    
    //ADC Configuration
    adc_init();
    adc_gpio_init(PIN_ADC_I);//I channel (0) - configure pin for ADC use
    adc_gpio_init(PIN_ADC_Q);//Q channel (1) - configure pin for ADC use
    adc_gpio_init(PIN_BATTERY);//Battery - configure pin for ADC use

    adc_set_temp_sensor_enabled(true);
    adc_set_clkdiv(99); //48e6/480e3

    //Configure PIN_PTT
    gpio_init(PIN_PTT);
    gpio_set_function(PIN_PTT, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_PTT, GPIO_IN);
    gpio_pull_up(PIN_PTT);
    gpio_init(LED);
    gpio_set_function(LED, GPIO_FUNC_SIO);
    gpio_set_dir(LED, GPIO_OUT);

    //drive RF and magnitude pin to zero to make sure they are switched off
        gpio_init(PIN_MAGNITUDE);
gpio_set_function(PIN_MAGNITUDE, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_MAGNITUDE, GPIO_OUT);
    gpio_put(PIN_MAGNITUDE, 0);
    gpio_set_function(PIN_RF, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_RF, GPIO_OUT);
    gpio_put(PIN_RF, 0);

    //band select
    gpio_init(PIN_BAND_0);//band 0
    gpio_init(PIN_BAND_1);//band 1
    gpio_init(PIN_BAND_2);//band 2
    gpio_set_function(PIN_BAND_0, GPIO_FUNC_SIO);
    gpio_set_function(PIN_BAND_1, GPIO_FUNC_SIO);
    gpio_set_function(PIN_BAND_2, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_BAND_0, GPIO_OUT);
    gpio_set_dir(PIN_BAND_1, GPIO_OUT);
    gpio_set_dir(PIN_BAND_2, GPIO_OUT);
    
    // Configure DMA for ADC transfers
    adc_dma_ping = dma_claim_unused_channel(true);
    adc_dma_pong = dma_claim_unused_channel(true);
    ping_cfg = dma_channel_get_default_config(adc_dma_ping);
    pong_cfg = dma_channel_get_default_config(adc_dma_pong);

    channel_config_set_transfer_data_size(&ping_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&ping_cfg, false);
    channel_config_set_write_increment(&ping_cfg, true);
    channel_config_set_dreq(&ping_cfg, DREQ_ADC);// Pace transfers based on availability of ADC samples
    channel_config_set_chain_to(&ping_cfg, adc_dma_pong);

    channel_config_set_transfer_data_size(&pong_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&pong_cfg, false);
    channel_config_set_write_increment(&pong_cfg, true);
    channel_config_set_dreq(&pong_cfg, DREQ_ADC);// Pace transfers based on availability of ADC samples
    channel_config_set_chain_to(&pong_cfg, adc_dma_ping);

    //settings semaphore
    sem_init(&settings_semaphore, 1, 1);

    pwm_audio_sink_init();

    //configure DMA for audio transfers
    capture_dma = dma_claim_unused_channel(true);
    capture_cfg = dma_channel_get_default_config(capture_dma);
    channel_config_set_transfer_data_size(&capture_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&capture_cfg, true);
    channel_config_set_write_increment(&capture_cfg, true);

    dma_set_irq0_channel_mask_enabled((1u<<adc_dma_ping) | (1u<<adc_dma_pong), true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);


}

static void on_usb_audio_tx_ready()
{
  uint8_t usb_buf[SAMPLE_BUFFER_SIZE * sizeof(int16_t)] = {0};

  // Callback from TinyUSB library when all data is ready
  // to be transmitted.
  //
  // Write local buffer to the USB microphone
  ring_buffer_pop(&usb_ring_buffer, usb_buf, sizeof(usb_buf));
  usb_audio_device_write(usb_buf, sizeof(usb_buf));
}
void rx::pwm_ramp_down()
{
  /* //generated a raised cosine slope to move between VCC/2 and 0
  uint32_t frequency_Hz = 1u;
  uint32_t phase_increment = ((uint64_t)frequency_Hz<<32u)/audio_sample_rate;
  uint32_t phase = (1u<<30u); //90 degrees
  uint32_t num_samples = audio_sample_rate/(frequency_Hz*2u);//half a cycle

  int16_t level;
  for(uint32_t sample = 0; sample<num_samples; sample++)
  {
    level = (((int32_t)sin_table[phase>>21]*(int32_t)pwm_max)>>17) + (int32_t)pwm_max/4;
    level = std::min(level, (int16_t)pwm_max);
    level = std::max(level, (int16_t)0);
    phase += phase_increment;
    pwm_set_gpio_level(16, level);
  } */
}

void rx::pwm_ramp_up()
{
  /* //generated a raised cosine slope to move between 0 and VCC/2
  uint32_t frequency_Hz = 1u;
  uint32_t phase_increment = ((uint64_t)frequency_Hz<<32u)/audio_sample_rate;
  uint32_t phase = -(1u<<30u); //90 degrees
  uint32_t num_samples = audio_sample_rate/(frequency_Hz*2u);//half a cycle

  int16_t level;
  for(uint32_t sample = 0; sample<num_samples; sample++)
  {
    level = (((int32_t)sin_table[phase>>21]*(int32_t)pwm_max)>>17) + (int32_t)pwm_max/4;
    level = std::min(level, (int16_t)pwm_max);
    level = std::max(level, (int16_t)0);
    phase += phase_increment;
    pwm_set_gpio_level(16, level);
  } */
}

bool __not_in_flash_func(rx::ptt)()
{
  static uint16_t timer = 0;
  
  //while transmitting this gets called about 10000 times per second
  if((dit.is_keyed() || dah.is_keyed()) && (transmit_mode == CW)) timer = 5000;
  else if(timer) timer--;
  // Debugging print statements
  printf("PTT: dit.is_keyed() = %d, dah.is_keyed() = %d, timer = %d, GPIO = %d\n", dit.is_keyed(), dah.is_keyed(), timer, gpio_get(PIN_PTT));
  if(timer != 0) //force ptt because dit/dah is recently keyed
  {
    gpio_set_dir(PIN_PTT, GPIO_OUT);
    gpio_put(PIN_PTT, 0);
  }
  else
  {
    gpio_set_dir(PIN_PTT, GPIO_IN);
  }
  sleep_us(1);

  return dit.is_keyed() || dah.is_keyed();
}

void rx::read_batt_temp()
{
  adc_select_input(3);
  battery = 0;
  for(uint8_t i=0; i<16; i++)
  {
    battery += adc_read();
  }
  adc_select_input(4);
  temp = 0;
  for(uint8_t i=0; i<16; i++)
  {
    temp += adc_read();
  }
}

static bool __not_in_flash_func(usb_callback)(repeating_timer_t *rt)
{
  usb_audio_device_task();
  return true; // keep repeating
}

void rx::set_alarm_pool(alarm_pool_t *p)
{
  pool = p;
}

critical_section_t usb_volumute;
static int16_t usb_volume = 32767;  // usb volume
static bool usb_mute = false;     // usb mute control

// usb mute setting = true is muted
static void on_usb_set_mutevol(bool mute, int16_t vol)
{
  //printf ("usbcb: got mute %d vol %d\n", mute, vol);
  critical_section_enter_blocking(&usb_volumute);
  usb_volume = 32767 * powf(10, (float)vol / (20 * 256));
  usb_mute = mute;
  critical_section_exit(&usb_volumute);
}

//thread safe method to get raw IQ data
bool rx::get_raw_data(int16_t &i, int16_t &q)
{
  return rx_dsp_inst.get_raw_data(i, q);
}

uint32_t rx::get_iq_buffer_level()
{
  return rx_dsp_inst.get_iq_buffer_level();
}

void __not_in_flash_func(rx::process_block)(uint16_t adc_samples[], int16_t audio[])
{
  //capture usb volume and mute settings
  critical_section_enter_blocking(&usb_volumute);
  int32_t safe_usb_volume = usb_volume;
  bool safe_usb_mute = usb_mute;
  critical_section_exit(&usb_volumute);

  //process adc IQ samples to produce raw audio
  int16_t usb_audio[adc_block_size/decimation_rate];
  uint16_t num_samples = rx_dsp_inst.process_block(
      adc_samples, audio, stream_raw_iq ? &usb_ring_buffer : NULL);

  for(uint16_t idx=0; idx<num_samples; ++idx)
  {
    //usb audio volume is controlled from usb
    if (safe_usb_mute) {
      usb_audio[idx] = 0;
    } else {
      usb_audio[idx] = (audio[idx] * safe_usb_volume) / 32767;
    }
  }

  if (!stream_raw_iq) {
    // add usb audio to ring buffer
    int16_t tmp_audio[2 * num_samples];
    for (uint16_t idx = 0; idx < num_samples; idx++) {
      tmp_audio[2 * idx] = usb_audio[idx];
      tmp_audio[2 * idx + 1] = usb_audio[idx];
    }
    ring_buffer_push_ovr(&usb_ring_buffer, (uint8_t *)tmp_audio,
                         sizeof(int16_t) * 2 * num_samples);
  }
}


void __not_in_flash_func(rx::transmit)()
{
    printf("Transmit Function Called\n");
    gpio_set_function(MAGNITUDE_PIN, GPIO_FUNC_PWM);
    gpio_set_function(PIN_RF, GPIO_FUNC_PIO0);

    const double clock_frequency_Hz = system_clock_rate;

    const float sample_rates[] = {
        12e3, //AM = 0u;
        12e3, //AMSYNC = 1u;
        10e3, //LSB = 2u;
        10e3, //USB = 3u;
        15e3, //FM = 4u;
        10e3, //CW = 5u;
    };

    // Use ADC to capture MIC input
    adc mic_adc(PIN_MIC, 2);

    // Use PWM to output magnitude
    pwm magnitude_pwm(PIN_MAGNITUDE);

    // Use PIO to output phase/frequency controlled oscillator
    transmit_nco rf_nco(PIN_RF, clock_frequency_Hz, tuned_frequency_Hz);
    const double sample_frequency_Hz = sample_rates[transmit_mode];
    const uint8_t waveforms_per_sample =
    rf_nco.get_waveforms_per_sample(clock_frequency_Hz, sample_frequency_Hz);

    // create modulator
    modulator audio_modulator;

    // scale FM deviation
    const double fm_deviation_Hz = 2.5e3;
    const uint32_t fm_deviation_f15 =
        round(2 * 32768.0 * fm_deviation_Hz /
              rf_nco.get_sample_frequency_Hz(clock_frequency_Hz, waveforms_per_sample));


    //create CW keyer
    cw_keyer keyer(tx_cw_paddle, tx_cw_speed, rf_nco.get_sample_frequency_Hz(clock_frequency_Hz, waveforms_per_sample), dit, dah);

    //mic gain
    //uint16_t scaled_mic_gain = 16 << tx_mic_gain;

    //test tone
    uint32_t test_tone_phase = 0;
    uint32_t test_tone_frequency_steps = pow(2, 32) * 100 * test_tone_frequency / sample_frequency_Hz;

    int32_t audio = 0;
    int32_t pwm_audio = 0;
    uint16_t magnitude = 0;
    int16_t phase = 0;
    int16_t i = 0; // not used in this design
    int16_t q = 0; // not used in this design
    int16_t audio_samples[PWM_AUDIO_NUM_SAMPLES];
    int audio_buff_idx = 0;
    gpio_put(LED, 1);
    while (ptt()) {
      
      if(test_tone_enable)
      {
        audio = sin_table[test_tone_phase >> 21];
        test_tone_phase += test_tone_frequency_steps;
        audio = audio;
      }
      else
      {
        audio = keyer.get_sample();
        pwm_audio = sin_table[test_tone_phase >> 21];
        test_tone_phase += test_tone_frequency_steps;
        pwm_audio = audio;
      if(audio_buff_idx >= PWM_AUDIO_NUM_SAMPLES) {
          printf("PWM_AUDIO_SINK_TX_PUSH==============\n");
          pwm_audio_sink_push(audio_samples, settings_to_apply.volume);
          audio_buff_idx = 0;
      } else {
        audio_samples[audio_buff_idx++] = pwm_audio;
        printf("AUDIO_PWM_SINK_ADD_ENTRY===========%ld\n", audio);
      }
        /* if(transmit_mode == CW)
        {
        } */
        /* else
        {
          // read audio from mic
          audio = mic_adc.get_sample() * scaled_mic_gain;
          audio = std::max((int32_t)-32767, std::min((int32_t)32767, audio));
        } */
      }
      tx_audio_level = tx_audio_level - (tx_audio_level >> 5) + (abs(audio) >> 5);

      // demodulate
      audio_modulator.process_sample(transmit_mode, audio, i, q, magnitude, phase, fm_deviation_f15);

      // output magnitude
      magnitude_pwm.output_sample(magnitude, settings_to_apply.pwm_min, settings_to_apply.pwm_max, settings_to_apply.pwm_threshold);

      // output phase // TODO: USE SI5351
      //rf_nco.output_sample(phase, waveforms_per_sample);
      
      //update_status
      update_status();
    }
    gpio_put(LED, 0);
    gpio_set_function(MAGNITUDE_PIN, GPIO_FUNC_SIO);
    gpio_set_function(PIN_RF, GPIO_FUNC_SIO);

}


void rx::run()
{
    usb_audio_device_init();
    critical_section_init(&usb_volumute);
    usb_audio_device_set_tx_ready_handler(on_usb_audio_tx_ready);
    usb_audio_device_set_mutevol_handler(on_usb_set_mutevol);
    repeating_timer_t usb_timer;
    hard_assert(pool);

    // here the delay theoretically should be 1067 (1ms = 1 / (15000 / 16))
    // however the 'usb_microphone_task' should be called more often, but not too often
    // to save compute
    bool ret = alarm_pool_add_repeating_timer_us(pool, 1067 / 2, usb_callback, NULL, &usb_timer);
    hard_assert(ret);

    while(true)
    {
      if(settings_changed) apply_settings();


      //read other adc channels when streaming is not running
      uint32_t timeout = 15000;
      read_batt_temp();

      //supress audio output until first block has completed
      audio_running = false;
      hw_clear_bits(&adc_hw->fcs, ADC_FCS_UNDER_BITS);
      hw_clear_bits(&adc_hw->fcs, ADC_FCS_OVER_BITS);
      adc_set_clkdiv(100 - 1);
      adc_fifo_setup(true, true, 1, false, false);
      adc_select_input(0);
      adc_set_round_robin(3);
      dma_channel_configure(adc_dma_ping, &ping_cfg, ping_samples, &adc_hw->fifo, adc_block_size, false);
      dma_channel_configure(adc_dma_pong, &pong_cfg, pong_samples, &adc_hw->fifo, adc_block_size, false);
      dma_channel_set_irq0_enabled(adc_dma_ping, true);
      dma_channel_set_irq0_enabled(adc_dma_pong, true);
      dma_start_channel_mask(1u << adc_dma_ping);
      adc_run(true);

      pwm_audio_sink_start();

      while(true)
      {
          //exchange data with UI (runing in core 0)
          update_status();

          //periodically (or when requested) suspend streaming
          if(timeout-- == 0 || suspend || settings_changed || ptt())
          {

            dma_channel_cleanup(adc_dma_ping);
            dma_channel_cleanup(adc_dma_pong);
            //pwm_audio_sink_stop();

            adc_run(false);
            adc_fifo_drain();
            adc_set_round_robin(0);
            adc_fifo_setup(false, false, 1, false, false);
            break;
          }

          //process adc data as each block completes
          int16_t audio[PWM_AUDIO_NUM_SAMPLES];
          dma_channel_wait_for_finish_blocking(adc_dma_ping);
          uint32_t start_time = time_us_32();
          process_block(ping_samples, audio);
          busy_time = pwm_audio_sink_push(audio, gain_numerator);
          busy_time -= start_time;
          dma_channel_wait_for_finish_blocking(adc_dma_pong);
          process_block(pong_samples, audio);
          pwm_audio_sink_push(audio, gain_numerator);
      }

      //suspended state
      if(suspend)
      {
        while(true)
        {
            update_status();

            //wait here if receiver is suspended
            if(!suspend)
            {
              break;
            }
        }
      }

      
      if(ptt())
      {

        transmit();

        //enable RX NCO
        pio_sm_set_enabled(pio, sm, true);
      }


    }
}
