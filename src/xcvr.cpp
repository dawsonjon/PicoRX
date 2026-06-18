#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include <string.h>
#include <algorithm>

#include "xcvr.h"
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
#include "transmit/iq_pwm.h"
#include "transmit/transmit_pico2_nco.h"
#include "transmit/tx_best_clock.h"
#include "transmit/speech_processor.h"
#include "transmit/modulator.h"
#include "transmit/cw_keyer.h"

//ring buffer for USB data
#define USB_BUF_SIZE (sizeof(int16_t) * 8 * (1 + (adc_block_size/decimation_rate)))
static ring_buffer_t usb_ring_buffer;
static uint8_t usb_buf[USB_BUF_SIZE];

//buffers and dma for ADC
int xcvr::adc_dma_ping;
int xcvr::adc_dma_pong;
dma_channel_config xcvr::ping_cfg;
dma_channel_config xcvr::pong_cfg;
uint16_t xcvr::ping_samples[adc_block_size];
uint16_t xcvr::pong_samples[adc_block_size];

bool xcvr::audio_running;

//dma for capture
int xcvr::capture_dma;
dma_channel_config xcvr::capture_cfg;

void xcvr::dma_handler() {


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


void xcvr::access(bool s)
{
  sem_acquire_blocking(&settings_semaphore);
  settings_changed |= s;
}

//r-etunes the quadrature NCO when operating in IQ mode
//also restores the RX tuning after TX in internal polar mode
void xcvr::tune_tx(bool transmit_enable)
{

  if(transmit_enable) {
    if(external_nco_active)
    {
      double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+ppm);
      sem_acquire_blocking(&i2c_semaphore);
      nco_frequency_Hz = external_nco.set_frequency_hz(adjusted_tuned_frequency_Hz);
      sem_release(&i2c_semaphore);
      offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
    }
    else
    {
      double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+ppm);
      nco_frequency_Hz = nco_set_frequency(pio, sm, adjusted_tuned_frequency_Hz, system_clock_rate, 0, if_mode);
      offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
      pwm_audio_sink_update_pwm_max((system_clock_rate/pwm_audio_sample_rate)-1);
    }
  } else {
    if(external_nco_active)
    {
      double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+ppm);
      sem_acquire_blocking(&i2c_semaphore);
      nco_frequency_Hz = external_nco.set_frequency_hz(adjusted_tuned_frequency_Hz + ((uint16_t)if_frequency_hz_over_100*100));
      sem_release(&i2c_semaphore);
      offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);
      rx_dsp_inst.amsync_reset();
    }
    else
    {
      double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+ppm);
      nco_frequency_Hz = nco_set_frequency(pio, sm, adjusted_tuned_frequency_Hz, system_clock_rate, if_frequency_hz_over_100, if_mode);
      offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
      pwm_audio_sink_update_pwm_max((system_clock_rate/pwm_audio_sample_rate)-1);
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);
      rx_dsp_inst.amsync_reset();
    }
  }

}

void xcvr::tune_rx()
{
  if(enable_external_nco)
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
      rx_dsp_inst.amsync_reset();
    }

    //initialise external nco before first use
    if(!external_nco_initialised)
    {
      external_nco_good = external_nco.initialise(i2c1, PIN_DISPLAY_SDA, PIN_DISPLAY_SCL, 0x60, 25000000);
      external_nco.set_drive(3);
      external_nco.crystal_load(3);
      external_nco.start_rx();
      external_nco_initialised = true;
    }

    //start external oscillator each time it is enabled
    if(!external_nco_active)
    {
      external_nco.start_rx();
      external_nco_active = true;
    }

    if(external_nco_good)
    {
        double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+ppm);
        sem_acquire_blocking(&i2c_semaphore);
        nco_frequency_Hz = external_nco.set_frequency_hz(adjusted_tuned_frequency_Hz + ((uint16_t)if_frequency_hz_over_100*100));
        sem_release(&i2c_semaphore);
        offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
        rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);
        rx_dsp_inst.amsync_reset();
        m_needs_tune = false;
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

    disable_pwm();
    if(pwm_is_disabled()) {
      double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+ppm);
      nco_frequency_Hz = nco_set_frequency(pio, sm, adjusted_tuned_frequency_Hz, system_clock_rate, if_frequency_hz_over_100, if_mode);
      offset_frequency_Hz = adjusted_tuned_frequency_Hz - nco_frequency_Hz;
      pwm_audio_sink_update_pwm_max((system_clock_rate/pwm_audio_sample_rate)-1);
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);
      rx_dsp_inst.amsync_reset();
      enable_pwm();
      m_needs_tune = false;
    }
  }

}

void xcvr::tx_update_status()
{

   const bool sem_acquired = sem_try_acquire(&settings_semaphore);
   if(sem_acquired)
   {
     status.audio_level = tx_audio_level;
     status.transmitting = true;
     sem_release(&settings_semaphore);
   }
}

void xcvr::release()
{
  sem_release(&settings_semaphore);
}

void xcvr::update_status()
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
     status.transmitting = false;

     sem_release(&settings_semaphore);
   }
}

void xcvr::apply_settings()
{

   if(sem_try_acquire(&settings_semaphore))
   {

      if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.band_7_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 0);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.band_6_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 1);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.band_5_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 0);
        gpio_put(PIN_BAND_1, 1);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.band_4_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 1);
        gpio_put(PIN_BAND_1, 1);
        gpio_put(PIN_BAND_2, 0);
      }
      else if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.band_3_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 0);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 1);
      }
      else if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.band_2_limit * 125000))
      {
        gpio_put(PIN_BAND_0, 1);
        gpio_put(PIN_BAND_1, 0);
        gpio_put(PIN_BAND_2, 1);
      }
      else if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.band_1_limit * 125000))
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

      tx_enable = false;
      for(int band=0; band < NUM_BANDS; ++band) {
        if(settings_to_apply.tuned_frequency_Hz > (settings_to_apply.tx_band_limits.lower[band] * 50000) &&
           settings_to_apply.tuned_frequency_Hz < (settings_to_apply.tx_band_limits.upper[band] * 50000)) {
          tx_enable = true;
        }
      }

      //apply frequency offset
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);

      //apply CW sidetone
      rx_dsp_inst.set_cw_sidetone_Hz(settings_to_apply.cw_sidetone_Hz);
      cw_sidetone_frequency_Hz = settings_to_apply.cw_sidetone_Hz;

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

      //apply impulse blanker threshold
      rx_dsp_inst.set_impulse_threshold(settings_to_apply.impulse_threshold);

      //apply squelch
      rx_dsp_inst.set_squelch(settings_to_apply.squelch_threshold, settings_to_apply.squelch_timeout);

      //apply swap iq
      rx_dsp_inst.set_swap_iq(settings_to_apply.swap_iq);

      //apply iq imbalance correction
      rx_dsp_inst.set_iq_correction(settings_to_apply.iq_correction);

      //apply transmit settings
      transmit_mode = settings_to_apply.mode;
      test_tone_setting = settings_to_apply.test_tone_setting;
      test_tone_frequency = settings_to_apply.test_tone_frequency;
      tx_cw_paddle = settings_to_apply.cw_paddle;
      tx_cw_speed = settings_to_apply.cw_speed;
      tx_mic_gain = settings_to_apply.mic_gain;
      tx_modulation = settings_to_apply.tx_modulation;
      tx_pwm_min = settings_to_apply.pwm_min;
      tx_pwm_max = settings_to_apply.pwm_max;
      tx_pwm_threshold = settings_to_apply.pwm_threshold;
      tx_use_best_clock = settings_to_apply.tx_use_best_clock;
      tx_monitor = settings_to_apply.tx_monitor;
      tx_noise_gate = settings_to_apply.tx_noise_gate;
      tx_treble = settings_to_apply.tx_treble;
      tx_bass = settings_to_apply.tx_bass;
      tx_compression = settings_to_apply.tx_compression;
      tx_band_limits = settings_to_apply.tx_band_limits;
      tx_phase_dither = settings_to_apply.tx_phase_dither;
      tx_i_offset = settings_to_apply.tx_i_offset;
      tx_q_offset = settings_to_apply.tx_q_offset;
      tx_iq_balance = settings_to_apply.tx_iq_balance;
      stream_raw_iq = settings_to_apply.stream_raw_iq;

      if((tuned_frequency_Hz != settings_to_apply.tuned_frequency_Hz) ||
         (ppm != settings_to_apply.ppm) ||
         (if_mode != settings_to_apply.if_mode) ||
         (if_frequency_hz_over_100 != settings_to_apply.if_frequency_hz_over_100) ||
         (enable_external_nco != settings_to_apply.enable_external_nco))
      {

        tuned_frequency_Hz = settings_to_apply.tuned_frequency_Hz;
        ppm = settings_to_apply.ppm;
        if_mode = settings_to_apply.if_mode;
        if_frequency_hz_over_100 = settings_to_apply.if_frequency_hz_over_100;
        enable_external_nco = settings_to_apply.enable_external_nco;
        m_needs_tune = true;
      }

      settings_changed = false;
      sem_release(&settings_semaphore);
   }

}

void xcvr::get_spectrum(uint8_t spectrum[], uint8_t &dB10, uint8_t zoom)
{
  rx_dsp_inst.get_spectrum(spectrum, dB10, zoom);
}

void xcvr::get_audio(uint8_t audio[])
{
  rx_dsp_inst.get_audio_capture(audio);
}

xcvr::xcvr(xcvr_settings & _settings_to_apply, xcvr_status & _status) : dit(PIN_DIT), dah(PIN_DAH), settings_to_apply(_settings_to_apply), status(_status)
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
    adc_gpio_init(26);//I channel (0) - configure pin for ADC use
    adc_gpio_init(27);//Q channel (1) - configure pin for ADC use
    adc_gpio_init(29);//Battery - configure pin for ADC use
    adc_set_temp_sensor_enabled(true);
    adc_set_clkdiv(99); //48e6/480e3

    //Configure PTT
    gpio_init(PIN_PTT);
    gpio_set_function(PIN_PTT, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_PTT, GPIO_OUT);
    gpio_put(PIN_PTT, 1);
    gpio_pull_up(PIN_PTT);
    gpio_init(LED);
    gpio_set_function(LED, GPIO_FUNC_SIO);
    gpio_set_dir(LED, GPIO_OUT);

    //drive RF and magnitude pin to zero to make sure they are switched off
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
    sem_init(&i2c_semaphore, 1, 1);

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

void xcvr::read_batt_temp()
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
  (void)rt;
  usb_audio_device_task();
  return true; // keep repeating
}

void xcvr::set_alarm_pool(alarm_pool_t *p)
{
  pool = p;
}

critical_section_t usb_volumute;
static int16_t usb_volume = 32767;  // usb volume
static bool usb_mute = false;     // usb mute control

// usb mute setting = true is muted
static void on_usb_set_mutevol(bool mute, int16_t vol)
{
  critical_section_enter_blocking(&usb_volumute);
  usb_volume = 32767 * powf(10, (float)vol / (20 * 256));
  usb_mute = mute;
  critical_section_exit(&usb_volumute);
}

static void on_usb_audio_tx_ready()
{
  uint16_t _usb_buf[SAMPLE_BUFFER_SIZE] = {0};

  ring_buffer_pop(&usb_ring_buffer, (uint8_t *)_usb_buf, sizeof(_usb_buf));
  usb_audio_device_write(_usb_buf, sizeof(_usb_buf));
}

//thread safe method to get raw IQ data
bool xcvr::get_raw_data(int16_t &i, int16_t &q)
{
  return rx_dsp_inst.get_raw_data(i, q);
}

uint32_t xcvr::get_iq_buffer_level()
{
  return rx_dsp_inst.get_iq_buffer_level();
}

void __not_in_flash_func(xcvr::process_block)(uint16_t adc_samples[], int16_t audio[])
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
  hard_assert(num_samples <= (adc_block_size / decimation_rate));

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
    int16_t tmp_audio[2 * (adc_block_size / decimation_rate)];
    for (uint16_t idx = 0; idx < num_samples; idx++) {
      tmp_audio[2 * idx] = usb_audio[idx];
      tmp_audio[2 * idx + 1] = usb_audio[idx];
    }
    ring_buffer_push_ovr(&usb_ring_buffer, (uint8_t *)tmp_audio,
                         sizeof(int16_t) * 2 * num_samples);
  }
}

bool __not_in_flash_func(xcvr::ptt)()
{
  if(!tx_enable) return false;
  static uint16_t timer = 0;
  if(dit.is_keyed() || dah.is_keyed()) timer = 5000;
  else if(timer) timer--;

  return timer;
}

void xcvr::begin_signal_generator(const double sample_frequency_Hz)
{
    m_test_tone_frequency_steps = pow(2, 32) * 100 * test_tone_frequency / sample_frequency_Hz;
    m_test_tone1_frequency_steps = pow(2, 32) * 800 / sample_frequency_Hz;
    m_test_tone2_frequency_steps = pow(2, 32) * 1200 / sample_frequency_Hz;
    m_side_tone_frequency_steps = pow(2, 32) * cw_sidetone_frequency_Hz / sample_frequency_Hz;
    m_scaled_mic_gain = 16 << tx_mic_gain;
    if(tx_monitor) enable_pwm();
    tx_complete = false;
}

void xcvr::end_signal_generator()
{
    if(tx_monitor) disable_pwm();
}

int32_t __not_in_flash_func(xcvr::get_tx_sample)(adc &mic_adc, cw_keyer &keyer)
{

    int32_t audio = 0;
    int32_t monitor = 0;

    if(test_tone_setting == 1)
    {
      monitor = audio = sin_table[m_test_tone_phase >> 21];
      m_test_tone_phase += m_test_tone_frequency_steps;
    }
    else if(test_tone_setting == 2)
    {
      monitor = audio = sin_table[m_test_tone1_phase >> 21]/2 + sin_table[m_test_tone2_phase >> 21]/2;
      m_test_tone1_phase += m_test_tone1_frequency_steps;
      m_test_tone2_phase += m_test_tone2_frequency_steps;
    }
    else
    {
      if(transmit_mode == CW)
      {
        audio = keyer.get_sample();
        monitor = (int32_t)audio * (int32_t)sin_table[m_side_tone_phase >> 21] >> 16;
        m_side_tone_phase += m_side_tone_frequency_steps;
      }
      else
      {
        // read audio from mic
        audio = mic_adc.get_sample();
        audio *= m_scaled_mic_gain;
        audio = std::max((int32_t)-32767, std::min((int32_t)32767, audio));
        audio = process_speech(audio, tx_noise_gate, tx_treble, tx_bass);
        static float envelope=0;
        int16_t short_audio = audio;
        const uint16_t compression_lookup[6] = {32767, 16383, 8191, 4095, 2047, 1023};
        const uint16_t compression_threshold = compression_lookup[tx_compression];
        compress(short_audio, envelope, compression_threshold);
        monitor = audio = short_audio;
      }
    }
    tx_audio_level = tx_audio_level - (tx_audio_level >> 5) + (abs(audio) >> 5);

    //transmit monitor -  shut down PWM cleanly before returning to RX
    if(tx_monitor){
      if(!ptt()) disable_pwm();
      tx_complete = pwm_is_disabled();
      pwm_audio_sink_set_value(monitor, gain_numerator);
    } else {
      tx_complete = !ptt();
    }
    return audio;

}

//TRANSMIT
void __not_in_flash_func(xcvr::transmit_iq)()
{

    tune_tx(true);

    gpio_set_function(PIN_TX_I, GPIO_FUNC_PWM);
    gpio_set_function(PIN_TX_Q, GPIO_FUNC_PWM);

    const float sample_rates[] = {
        12e3, //AM = 0u;
        12e3, //AMSYNC = 1u;
        10e3, //LSB = 2u;
        10e3, //USB = 3u;
        15e3, //FM = 4u;
        10e3, //CW = 5u;
    };
    const double sample_frequency_Hz = sample_rates[transmit_mode];
    begin_signal_generator(sample_frequency_Hz);

    // Use ADC to capture MIC input
    adc mic_adc(PIN_MIC, 2);

    // Use PWM to output magnitude
    const double clock_frequency_Hz = system_clock_rate;
    iq_pwm iq_pwm_inst(PIN_TX_I, PIN_TX_Q, sample_frequency_Hz, clock_frequency_Hz);

    // create modulator
    modulator audio_modulator;

    // scale FM deviation
    const double fm_deviation_Hz = 2.5e3;
    const uint32_t fm_deviation_f15 = round(2 * 32768.0 * fm_deviation_Hz / sample_frequency_Hz);

    //create CW keyer
    cw_keyer keyer(tx_cw_paddle, tx_cw_speed, sample_frequency_Hz, dit, dah);

    const int32_t frequency_shift_steps = pow(2, 32) * offset_frequency_Hz / sample_frequency_Hz;
    uint32_t frequency_shift_phase = 0;

    int32_t audio = 0;
    uint16_t magnitude = 0;
    int16_t phase = 0;
    int16_t i = 0;
    int16_t q = 0;
    int32_t i_dc = 0;
    int32_t q_dc = 0;

    gpio_put(LED, 1);
    while (!tx_complete) {

      for(uint16_t idx=0; idx<1000; idx++)
      {
        //get audio sample from e.g. MIC
        audio = get_tx_sample(mic_adc, keyer);

        //Apply Modulation
        s_debug debug;
        audio_modulator.process_sample(transmit_mode, audio, i, q, magnitude, phase, fm_deviation_f15, debug);

        //Apply frequency shift
        const uint16_t scaled_phase = (frequency_shift_phase >> 21);
        const int16_t rotation_i =  sin_table[(scaled_phase+512u) & 0x7ff]; //32 - 21 = 11MSBs
        const int16_t rotation_q = -sin_table[scaled_phase & 0x7ff];
        frequency_shift_phase += frequency_shift_steps;
        const int16_t i_shifted = (((int32_t)i * rotation_i) - ((int32_t)q * rotation_q)) >> 15;
        const int16_t q_shifted = (((int32_t)q * rotation_i) + ((int32_t)i * rotation_q)) >> 15;

        //reduce to slightly less than full scale
        i = ((int32_t)i_shifted * 31000) >> 15;
        q = ((int32_t)q_shifted * 31000) >> 15;

        //Automatic DC removal
        i_dc = i_dc - (i_dc >> 8) + i;
        q_dc = q_dc - (q_dc >> 8) + q;
        i -= (i_dc >> 8);
        q -= (q_dc >> 8);

        //Manual DC offset
        i += tx_i_offset * 8;
        q += tx_q_offset * 8;

        //IQ Balance
        int32_t gain_q = 16384 + tx_iq_balance*32;
        q = ((int32_t)q * gain_q)>>14;

        //Output IQ
        iq_pwm_inst.output_sample(i, q);

      }

      //update_status
      tx_update_status();
    }

    //restore clock settings for receive
    end_signal_generator();
    tune_tx(false);

    gpio_put(LED, 0);
    gpio_set_function(PIN_TX_I, GPIO_FUNC_SIO);
    gpio_set_function(PIN_TX_Q, GPIO_FUNC_SIO);

}


void __not_in_flash_func(xcvr::transmit_polar_external)()
{

    gpio_set_function(PIN_MAGNITUDE, GPIO_FUNC_PWM);

    // Use ADC to capture MIC input
    adc mic_adc(PIN_MIC, 2);

    // Use PWM to output magnitude
    pwm magnitude_pwm(PIN_MAGNITUDE);

    // Use PIO to output phase/frequency controlled oscillator
    double sample_frequency_Hz = 8000;
    begin_signal_generator(sample_frequency_Hz);

    // create modulator
    modulator audio_modulator;

    // scale FM deviation
    const double fm_deviation_Hz = 2.5e3;
    const uint32_t fm_deviation_f15 =
        round(2 * 32768.0 * fm_deviation_Hz / sample_frequency_Hz);

    //create CW keyer
    cw_keyer keyer(tx_cw_paddle, tx_cw_speed, sample_frequency_Hz, dit, dah);

    int32_t audio = 0;
    uint16_t magnitude = 0;
    uint16_t last_magnitude = 0;
    int16_t phase = 0;
    uint16_t next_magnitude = 0;
    int16_t next_phase = 0;
    int16_t i = 0;
    int16_t q = 0;

    //external nco setup
    sem_acquire_blocking(&i2c_semaphore);
    double frequency_resolution_Hz = external_nco.set_tx_frequency_hz(tuned_frequency_Hz);
    external_nco.start_tx();
    int32_t frequency_steps_per_sample = round(sample_frequency_Hz/frequency_resolution_Hz);
    uint32_t centre_frequency = round(tuned_frequency_Hz/frequency_resolution_Hz);
    int16_t last_phase_f16 = 0;
    int32_t frequency_steps = 0;
    bool toggle = false;
    uint16_t period_us = round(1.0e6/sample_frequency_Hz);
    uint64_t next = time_us_64() + period_us;
    const uint16_t MAG_THR = 200;

    gpio_put(LED, 1);
    while (!tx_complete) {

      for(uint16_t idx=0; idx<1000; idx++)
      {
        //get audio sample from e.g. MIC
        audio = get_tx_sample(mic_adc, keyer);

        // demodulate
        s_debug debug;
        audio_modulator.process_sample(transmit_mode, audio, i, q, next_magnitude, next_phase, fm_deviation_f15, debug);

        // output phase
        if(toggle){ //only output at half rate when using external nco


            int16_t phase_change_f16 = 0;

            if (magnitude >= MAG_THR) { //good phase
                phase_change_f16 = phase - last_phase_f16;
                frequency_steps = (phase_change_f16 * frequency_steps_per_sample) >> 16;
            } else { //bad phase
              if (next_magnitude >= MAG_THR) { //next phase good
                phase_change_f16 = (next_phase - last_phase_f16);
                uint16_t positive_phase_change_f16 = phase_change_f16; //force a positive number half way to next phase
                phase_change_f16 = positive_phase_change_f16/2;
                frequency_steps = (phase_change_f16 * frequency_steps_per_sample) >> 16;
              }
            }

            // save phase predictor
            external_nco.set_tx_freq_adjustment(centre_frequency + frequency_steps);
            last_phase_f16 += (frequency_steps << 16) / frequency_steps_per_sample;

            // output magnitude
            magnitude_pwm.output_sample(last_magnitude, tx_pwm_min, tx_pwm_max, tx_pwm_threshold);
            last_magnitude = magnitude;

            magnitude = next_magnitude;
            phase = next_phase;
        }

        toggle += 1;

        while (time_us_64() < next) { tight_loop_contents(); }
        next += period_us;

      }

      //update_status
      tx_update_status();
    }

    end_signal_generator();
    external_nco.start_rx();
    //external nco
    sem_release(&i2c_semaphore);

    gpio_put(LED, 0);
    gpio_set_function(PIN_MAGNITUDE, GPIO_FUNC_SIO);

}

void __not_in_flash_func(xcvr::transmit_polar)()
{

    gpio_set_function(PIN_MAGNITUDE, GPIO_FUNC_PWM);
    gpio_set_function(PIN_RF, GPIO_FUNC_PIO0);

    double adjusted_tuned_frequency_Hz = tuned_frequency_Hz * 1e6/(1e6+ppm);
    if(tx_use_best_clock) {
      system_clock_rate = tx_best_clock(adjusted_tuned_frequency_Hz);
    }
    pwm_audio_sink_update_pwm_max((system_clock_rate/pwm_audio_sample_rate)-1);
    const double clock_frequency_Hz = system_clock_rate;

    const float sample_rates[] = {
        12e3, //AM = 0u;
        12e3, //AMSYNC = 1u;
        9.6e3, //LSB = 2u;
        9.6e3, //USB = 3u;
        15e3, //FM = 4u;
        9.6e3, //CW = 5u;
    };

    // Use ADC to capture MIC input
    adc mic_adc(PIN_MIC, 2);

    // Use PWM to output magnitude
    pwm magnitude_pwm(PIN_MAGNITUDE);

    // Use PIO to output phase/frequency controlled oscillator
    transmit_nco rf_nco(PIN_RF, clock_frequency_Hz, adjusted_tuned_frequency_Hz, tx_phase_dither);
    const double sample_frequency_Hz = sample_rates[transmit_mode];
    begin_signal_generator(sample_frequency_Hz);
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

    int32_t audio = 0;
    uint16_t magnitude = 0;
    uint16_t prev_magnitude = 0;
    //uint16_t prev_magnitude2 = 0;
    int16_t phase = 0;
    //int16_t last_phase = 0;
    //int16_t output_phase = 0;
    //int16_t frequency = 0;
    int16_t i = 0; // not used in this design
    int16_t q = 0; // not used in this design

    gpio_put(LED, 1);
    while (!tx_complete) {

      for(uint16_t idx=0; idx<1000; idx++)
      {
        //get audio sample from e.g. MIC
        audio = get_tx_sample(mic_adc, keyer);

        // demodulate
        s_debug debug;
        audio_modulator.process_sample(transmit_mode, audio, i, q, magnitude, phase, fm_deviation_f15, debug);

        // output magnitude
        magnitude_pwm.output_sample((magnitude+prev_magnitude)/2, tx_pwm_min, tx_pwm_max, tx_pwm_threshold);
        //prev_magnitude2 = prev_magnitude;
        prev_magnitude = magnitude;

        // output phase
        rf_nco.output_sample(phase, waveforms_per_sample);
      }

      //update_status
      tx_update_status();
    }

    //restore clock settings for receive
    end_signal_generator();
    tune_tx(false);

    gpio_put(LED, 0);
    gpio_set_function(PIN_MAGNITUDE, GPIO_FUNC_SIO);
    gpio_set_function(PIN_RF, GPIO_FUNC_SIO);

}

void xcvr::run()
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

      apply_settings();

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
          if(settings_changed) apply_settings();
          if(m_needs_tune) tune_rx();
          update_status();

          if(ptt()) disable_pwm();

          //periodically (or when requested) suspend streaming
          if(timeout-- == 0 || suspend || pwm_is_disabled())
          {

            dma_channel_cleanup(adc_dma_ping);
            dma_channel_cleanup(adc_dma_pong);
            pwm_audio_sink_stop();

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

        gpio_put(PIN_PTT, 0);
        if(tx_modulation){
          if(external_nco_active) {
            transmit_polar_external();
          } else {
            pio_sm_set_enabled(pio, sm, false);
            transmit_polar();
            pio_sm_set_enabled(pio, sm, true);
          }
        }
        else transmit_iq();
        gpio_put(PIN_PTT, 1);
        enable_pwm();

      }

    }
}
