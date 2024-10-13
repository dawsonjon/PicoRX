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

//ring buffer for USB data
#define USB_BUF_SIZE (sizeof(int16_t) * 2 * (1 + (adc_block_size/decimation_rate)))
static ring_buffer_t usb_ring_buffer;
static uint8_t usb_buf[USB_BUF_SIZE];

//buffers and dma for ADC
int rx::adc_dma_ping;
int rx::adc_dma_pong;
dma_channel_config rx::ping_cfg;
dma_channel_config rx::pong_cfg;
uint16_t rx::ping_samples[adc_block_size];
uint16_t rx::pong_samples[adc_block_size];

//buffers and dma for PWM audio output
int rx::audio_pwm_slice_num;
int rx::pwm_dma_ping;
int rx::pwm_dma_pong;
dma_channel_config rx::audio_ping_cfg;
dma_channel_config rx::audio_pong_cfg;
int16_t rx::ping_audio[adc_block_size];
int16_t rx::pong_audio[adc_block_size];
bool rx::audio_running;
uint16_t rx::num_ping_samples;
uint16_t rx::num_pong_samples;

//dma for capture
int rx::capture_dma;
dma_channel_config rx::capture_cfg;

void rx::dma_handler() {


    // adc ping             ####    ####
    // adc pong                 ####    ####
    // processing ping          ### 
    // processing pong              ###
    // pwm_ping                     ####
    // pwm_pong                         ####

    if(dma_hw->ints0 & (1u << adc_dma_ping))
    {
      dma_channel_configure(adc_dma_ping, &ping_cfg, ping_samples, &adc_hw->fifo, adc_block_size, false);
      if(audio_running){
        dma_channel_configure(pwm_dma_pong, &audio_pong_cfg, &pwm_hw->slice[audio_pwm_slice_num].cc, pong_audio, num_pong_samples, true);
      }
      dma_hw->ints0 = 1u << adc_dma_ping;
    }

    if(dma_hw->ints0 & (1u << adc_dma_pong))
    {
      dma_channel_configure(adc_dma_pong, &pong_cfg, pong_samples, &adc_hw->fifo, adc_block_size, false);
      dma_channel_configure(pwm_dma_ping, &audio_ping_cfg, &pwm_hw->slice[audio_pwm_slice_num].cc, ping_audio, num_ping_samples, true);
      if(!audio_running){
        audio_running = true;
      }
      dma_hw->ints0 = 1u << adc_dma_pong;
    }

}

void rx::access(bool s)
{
  sem_acquire_blocking(&settings_semaphore);
  settings_changed |= s;
}

void rx::release()
{
  sem_release(&settings_semaphore);
}

void rx::pwm_ramp_down()
{
  //generated a raised cosine slope to move between VCC/2 and 0
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
  }
}

void rx::pwm_ramp_up()
{
  //generated a raised cosine slope to move between 0 and VCC/2
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
  }
}

void rx::update_status()
{
   if(sem_try_acquire(&settings_semaphore))
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
     sem_release(&settings_semaphore);
   }
}

void rx::apply_settings()
{
   if(sem_try_acquire(&settings_semaphore))
   {

      //apply frequency
      tuned_frequency_Hz = settings_to_apply.tuned_frequency_Hz;

      //apply frequency calibration
      tuned_frequency_Hz *= 1e6/(1e6+settings_to_apply.ppm);

      uint32_t system_clock_rate;
      nco_frequency_Hz = nco_set_frequency(pio, sm, tuned_frequency_Hz, system_clock_rate);
      offset_frequency_Hz = tuned_frequency_Hz - nco_frequency_Hz;

      if(tuned_frequency_Hz > (settings_to_apply.band_7_limit * 125000))
      {
        gpio_put(2, 0);
        gpio_put(3, 0);
        gpio_put(4, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_6_limit * 125000))
      {
        gpio_put(2, 1);
        gpio_put(3, 0);
        gpio_put(4, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_5_limit * 125000))
      {
        gpio_put(2, 0);
        gpio_put(3, 1);
        gpio_put(4, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_4_limit * 125000))
      {
        gpio_put(2, 1);
        gpio_put(3, 1);
        gpio_put(4, 0);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_3_limit * 125000))
      {
        gpio_put(2, 0);
        gpio_put(3, 0);
        gpio_put(4, 1);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_2_limit * 125000))
      {
        gpio_put(2, 1);
        gpio_put(3, 0);
        gpio_put(4, 1);
      }
      else if(tuned_frequency_Hz > (settings_to_apply.band_1_limit * 125000))
      {
        gpio_put(2, 0);
        gpio_put(3, 1);
        gpio_put(4, 1);
      }
      else
      {
        gpio_put(2, 1);
        gpio_put(3, 1);
        gpio_put(4, 1);
      }

      //apply pwm_max
      pwm_max = (system_clock_rate/audio_sample_rate)-1;
      pwm_scale = 1+((INT16_MAX * 2)/pwm_max);
      pwm_set_wrap(audio_pwm_slice_num, pwm_max); 

      //apply frequency offset
      rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);

      //apply CW sidetone
      rx_dsp_inst.set_cw_sidetone_Hz(settings_to_apply.cw_sidetone_Hz);

      //apply gain calibration
      rx_dsp_inst.set_gain_cal_dB(settings_to_apply.gain_cal);

      //apply AGC speed
      rx_dsp_inst.set_agc_speed(settings_to_apply.agc_speed);

      //apply Automatic Notch Filter
      rx_dsp_inst.set_auto_notch(settings_to_apply.enable_auto_notch);

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

      //apply squelch
      rx_dsp_inst.set_squelch(settings_to_apply.squelch);

      //apply swap iq
      rx_dsp_inst.set_swap_iq(settings_to_apply.swap_iq);

      settings_changed = false;
      sem_release(&settings_semaphore);
   }
}

void rx::get_spectrum(uint8_t spectrum[], uint8_t &dB10)
{
  rx_dsp_inst.get_spectrum(spectrum, dB10);
}


rx::rx(rx_settings & settings_to_apply, rx_status & status) : settings_to_apply(settings_to_apply), status(status)
{

    settings_to_apply.suspend = false;
    suspend = false;

    //Configure PIO to act as quadrature oscilator
    pio = pio0;
    offset = pio_add_program(pio, &nco_program);
    sm = pio_claim_unused_sm(pio, true);
    nco_program_init(pio, sm, offset);
    ring_buffer_init(&usb_ring_buffer, usb_buf, USB_BUF_SIZE, 1);

    //configure SMPS into power save mode
    const uint PSU_PIN = 23;
    gpio_init(PSU_PIN);
    gpio_set_function(PSU_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(PSU_PIN, GPIO_OUT);
    gpio_put(PSU_PIN, 1);
    
    //ADC Configuration
    adc_init();
    adc_gpio_init(26);//I channel (0) - configure pin for ADC use
    adc_gpio_init(27);//Q channel (1) - configure pin for ADC use
    adc_gpio_init(29);//Battery - configure pin for ADC use
    adc_set_temp_sensor_enabled(true);
    adc_set_clkdiv(99); //48e6/480e3

    //band select
    gpio_init(2);//band 0
    gpio_init(3);//band 1
    gpio_init(4);//band 2
    gpio_set_function(2, GPIO_FUNC_SIO);
    gpio_set_function(3, GPIO_FUNC_SIO);
    gpio_set_function(4, GPIO_FUNC_SIO);
    gpio_set_dir(2, GPIO_OUT);
    gpio_set_dir(3, GPIO_OUT);
    gpio_set_dir(4, GPIO_OUT);
    
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

    //audio output
    const uint AUDIO_PIN = 16;
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    gpio_set_drive_strength(AUDIO_PIN, GPIO_DRIVE_STRENGTH_12MA);
    audio_pwm_slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f);
    pwm_max = 520;
    pwm_config_set_wrap(&config, pwm_max);
    pwm_init(audio_pwm_slice_num, &config, true);

    //configure DMA for audio transfers
    pwm_dma_ping = dma_claim_unused_channel(true);
    pwm_dma_pong = dma_claim_unused_channel(true);
    audio_ping_cfg = dma_channel_get_default_config(pwm_dma_ping);
    audio_pong_cfg = dma_channel_get_default_config(pwm_dma_pong);

    channel_config_set_transfer_data_size(&audio_ping_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&audio_ping_cfg, true);
    channel_config_set_write_increment(&audio_ping_cfg, false);
    channel_config_set_dreq(&audio_ping_cfg, DREQ_PWM_WRAP0 + audio_pwm_slice_num);

    channel_config_set_transfer_data_size(&audio_pong_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&audio_pong_cfg, true);
    channel_config_set_write_increment(&audio_pong_cfg, false);
    channel_config_set_dreq(&audio_pong_cfg, DREQ_PWM_WRAP0 + audio_pwm_slice_num);

    //configure DMA for audio transfers
    capture_dma = dma_claim_unused_channel(true);
    capture_cfg = dma_channel_get_default_config(pwm_dma_ping);
    channel_config_set_transfer_data_size(&capture_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&capture_cfg, true);
    channel_config_set_write_increment(&capture_cfg, true);

    dma_set_irq0_channel_mask_enabled((1u<<adc_dma_ping) | (1u<<adc_dma_pong), true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

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
static int16_t usb_volume=180;  // usb volume
static bool usb_mute = false;   // usb mute control

// usb mute setting = true is muted
static void on_usb_set_mutevol(bool mute, int16_t vol)
{
  //printf ("usbcb: got mute %d vol %d\n", mute, vol);
  critical_section_enter_blocking(&usb_volumute);
  uint16_t usb_volume = vol + 90; // defined as -90 to 90 => 0 to 180
  volume_steps = usb_volume * 32 / 180;
  usb_mute = mute;
  critical_section_exit(&usb_volumute);
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


uint16_t __not_in_flash_func(rx::process_block)(uint16_t adc_samples[], int16_t pwm_audio[])
{
  //capture usb volume and mute settings
  critical_section_enter_blocking(&usb_volumute);
  int32_t safe_usb_volume = usb_volume;
  bool safe_usb_mute = usb_mute;
  critical_section_exit(&usb_volumute);

  //process adc IQ samples to produce raw audio
  int16_t usb_audio[adc_block_size/decimation_rate];
  uint16_t num_samples = rx_dsp_inst.process_block(adc_samples, usb_audio);
  
  //post process audio for USB and PWM
  uint16_t odx = 0;
  for(uint16_t idx=0; idx<num_samples; ++idx)
  {
    int16_t audio = usb_audio[idx];

    //digital volume control
    audio = ((int32_t)audio * gain_numerator) >> 8;

    //convert to unsigned value in range 0 to 500 to output to PWM
    audio += INT16_MAX;
    audio = (uint16_t)audio/pwm_scale;

    //interpolate to PWM rate
    for(uint8_t subsample = 0; subsample < interpolation_rate; ++subsample)
    {
      pwm_audio[odx++] = audio;
    }

    //usb audio volume is controlled from usb
    if (safe_usb_mute) {
      usb_audio[idx] = 0;
    } else {
      usb_audio[idx] = (usb_audio[idx] * safe_usb_volume)/180;
    }
  }

  //add usb audio to ring buffer
  ring_buffer_push_ovr(&usb_ring_buffer, (uint8_t *)usb_audio, sizeof(int16_t) * num_samples); 
  return num_samples * interpolation_rate;
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
      if (settings_changed)
      {
        apply_settings();
        pwm_ramp_up();
      }

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

      while(true)
      {
          //exchange data with UI (runing in core 0)
          update_status();

          //periodically (or when requested) suspend streaming
          if(timeout-- == 0 || suspend || settings_changed)
          {

            dma_channel_cleanup(adc_dma_ping);
            dma_channel_cleanup(adc_dma_pong);
            dma_channel_cleanup(pwm_dma_ping);
            dma_channel_cleanup(pwm_dma_pong);

            adc_run(false);
            adc_fifo_drain();
            adc_set_round_robin(0);
            adc_fifo_setup(false, false, 1, false, false);

            if (settings_changed)
            {
              // slowly ramp down PWM to avoid pops
              pwm_ramp_down();
            }

            break;
          }

          //process adc data as each block completes
          dma_channel_wait_for_finish_blocking(adc_dma_ping);
          uint32_t start_time = time_us_32();
          num_ping_samples = process_block(ping_samples, ping_audio);
          busy_time = time_us_32()-start_time;
          dma_channel_wait_for_finish_blocking(adc_dma_pong);
          num_pong_samples = process_block(pong_samples, pong_audio);
      }

      //suspended state
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
}
