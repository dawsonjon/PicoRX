#include "pico/multicore.h"
#include <string.h>

#include "rx.h"


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
      gpio_put(14, 1);
      dma_channel_configure(adc_dma_ping, &ping_cfg, ping_samples, &adc_hw->fifo, adc_block_size, false);
      if(audio_running){
        dma_channel_configure(pwm_dma_pong, &audio_pong_cfg, &pwm_hw->slice[audio_pwm_slice_num].cc, pong_audio, num_pong_samples, true);
      }
      dma_hw->ints0 = 1u << adc_dma_ping;
    }

    if(dma_hw->ints0 & (1u << adc_dma_pong))
    {
      gpio_put(14, 0);
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
  settings_changed = s;
}

void rx::release()
{
  sem_release(&settings_semaphore);
}


void rx::apply_settings()
{
   if(sem_try_acquire(&settings_semaphore))
   {

      suspend = settings_to_apply.suspend;

      if(settings_changed)
      {
        //apply frequency
        tuned_frequency_Hz = settings_to_apply.tuned_frequency_Hz;
        nco_frequency_Hz = nco_program_init(pio, sm, offset, tuned_frequency_Hz-10000);
        offset_frequency_Hz = tuned_frequency_Hz - nco_frequency_Hz;
        rx_dsp_inst.set_frequency_offset_Hz(offset_frequency_Hz);

        //apply CW sidetone
        rx_dsp_inst.set_cw_sidetone_Hz(settings_to_apply.cw_sidetone_Hz);

        //apply AGC speed
        rx_dsp_inst.set_agc_speed(settings_to_apply.agc_speed);

        //apply mode
        rx_dsp_inst.set_mode(settings_to_apply.mode);

        //apply volume
        rx_dsp_inst.set_volume(settings_to_apply.volume);

        //apply squelch
        rx_dsp_inst.set_squelch(settings_to_apply.squelch);
      }

      //update status
      status.signal_strength_dBm = rx_dsp_inst.get_signal_strength_dBm();
      status.busy_time = busy_time;
      status.battery = battery;
      status.temp = temp;
      settings_changed = false;
      sem_release(&settings_semaphore);
   }
}

void rx::get_spectrum(float spectrum[], int16_t &offset)
{
  rx_dsp_inst.get_spectrum(spectrum, offset);
}

rx::rx(rx_settings & settings_to_apply, rx_status & status) : settings_to_apply(settings_to_apply), status(status)
{
    //Configure PIO to act as quadrature oscilator
    pio = pio0;
    offset = pio_add_program(pio, &hello_program);
    sm = pio_claim_unused_sm(pio, true);

    //configure SMPS into power save mode
    const uint PSU_PIN = 23;
    gpio_init(PSU_PIN);
    gpio_set_dir(PSU_PIN, GPIO_OUT);
    gpio_put(PSU_PIN, 1);
    
    //ADC Configuration
    adc_init();
    adc_gpio_init(26);//I channel (0) - configure pin for ADC use
    adc_gpio_init(27);//Q channel (1) - configure pin for ADC use
    adc_gpio_init(29);//Battery - configure pin for ADC use
    adc_set_temp_sensor_enabled(true);
    adc_set_clkdiv(0); //flat out
    
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

    dma_channel_configure(adc_dma_ping, &ping_cfg, ping_samples, &adc_hw->fifo, adc_block_size, false);
    dma_channel_configure(adc_dma_pong, &pong_cfg, pong_samples, &adc_hw->fifo, adc_block_size, false);

    //settings semaphore
    sem_init(&settings_semaphore, 1, 1);

    //audio output
    const uint AUDIO_PIN = 16;
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    audio_pwm_slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f); //125MHz
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

void rx::run()
{
    while(true)
    {
      //read other adc channels when streaming is not running
      uint32_t timeout = 1000;
      read_batt_temp();

      //supress audio output until first block has completed
      audio_running = false;
      adc_set_round_robin(3); //sample I/Q alternately
      adc_fifo_setup(true, true, 1, false, false);
      adc_set_round_robin(3);
      adc_select_input(0);
      adc_fifo_drain();
      adc_run(true);
      bool ping=true;
      dma_start_channel_mask(1u << adc_dma_ping);
      settings_changed = true;
      while(true)
      {
          //exchange data with UI (runing in core 0)
          apply_settings();

          //periodically (or when requested) suspend streaming
          if(timeout-- == 0 || suspend)
          {
            dma_channel_abort(adc_dma_ping);
            dma_channel_abort(adc_dma_pong);
            dma_channel_abort(pwm_dma_ping);
            dma_channel_abort(pwm_dma_pong);
            dma_channel_wait_for_finish_blocking(adc_dma_ping);
            dma_channel_wait_for_finish_blocking(adc_dma_pong);
            dma_channel_wait_for_finish_blocking(pwm_dma_ping);
            dma_channel_wait_for_finish_blocking(pwm_dma_pong);
            adc_run(false);
            adc_fifo_drain();
            adc_set_round_robin(0);
            adc_fifo_setup(false, false, 1, false, false);
            break;
          }

          //process adc data as each block completes
          clock_t start_time;
          dma_channel_wait_for_finish_blocking(adc_dma_ping);
          start_time = time_us_64();
          num_ping_samples = rx_dsp_inst.process_block(ping_samples, ping_audio);
          busy_time = time_us_64()-start_time;
          dma_channel_wait_for_finish_blocking(adc_dma_pong);
          num_pong_samples = rx_dsp_inst.process_block(pong_samples, pong_audio);
      }

      //suspended state
      while(true)
      {
          apply_settings();
          if(!suspend)
          {
            break;
          }
      }
    }
}
