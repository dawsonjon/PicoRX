#include "settings.h"
#include "autosave_memory.h"
#include "memory.h"
#include <hardware/flash.h>
#include "pico/multicore.h"
#include <cstring>

void apply_settings_to_xcvr(xcvr & transceiver, xcvr_settings & xcvr_settings, s_settings & settings, bool suspend, bool settings_changed)
{
  transceiver.access(settings_changed);
  xcvr_settings.tuned_frequency_Hz = settings.channel.frequency;
  xcvr_settings.agc_setting = settings.channel.agc_setting;
  xcvr_settings.agc_gain = settings.global.agc_gain;
  xcvr_settings.enable_auto_notch = settings.global.enable_auto_notch;
  xcvr_settings.enable_noise_reduction = settings.global.enable_noise_reduction;
  xcvr_settings.mode = settings.channel.mode;
  xcvr_settings.volume = settings.global.volume;
  xcvr_settings.squelch_threshold = settings.global.squelch_threshold;
  xcvr_settings.squelch_timeout = settings.global.squelch_timeout;
  xcvr_settings.step_Hz = step_sizes[settings.channel.step];
  xcvr_settings.cw_sidetone_Hz = settings.global.cw_sidetone*100;
  xcvr_settings.gain_cal = settings.global.gain_cal;
  xcvr_settings.suspend = suspend;
  xcvr_settings.swap_iq = settings.global.swap_iq;
  xcvr_settings.bandwidth = settings.channel.bandwidth;
  xcvr_settings.deemphasis = settings.global.deemphasis;
  xcvr_settings.band_1_limit = settings.global.band1;
  xcvr_settings.band_2_limit = settings.global.band2;
  xcvr_settings.band_3_limit = settings.global.band3;
  xcvr_settings.band_4_limit = settings.global.band4;
  xcvr_settings.band_5_limit = settings.global.band5;
  xcvr_settings.band_6_limit = settings.global.band6;
  xcvr_settings.band_7_limit = settings.global.band7;
  xcvr_settings.ppm = settings.global.ppm;
  xcvr_settings.iq_correction = settings.global.iq_correction;
  xcvr_settings.if_mode = settings.global.if_mode;
  xcvr_settings.if_frequency_hz_over_100 = settings.global.if_frequency_hz_over_100;
  xcvr_settings.noise_estimation = settings.global.noise_estimation + 8;
  xcvr_settings.noise_threshold = settings.global.noise_threshold;
  xcvr_settings.spectrum_smoothing = settings.global.spectrum_smoothing;
  xcvr_settings.enable_external_nco = settings.global.enable_external_nco;
  xcvr_settings.treble = settings.global.treble;
  xcvr_settings.bass = settings.global.bass;
  xcvr_settings.stream_raw_iq = settings.global.usb_stream;
  xcvr_settings.impulse_threshold = settings.global.impulse_threshold;

  xcvr_settings.test_tone_setting = settings.global.test_tone_setting;
  xcvr_settings.test_tone_frequency = settings.global.test_tone_frequency;
  xcvr_settings.cw_paddle = settings.global.cw_paddle;
  xcvr_settings.cw_speed = settings.global.cw_speed;
  xcvr_settings.mic_gain = settings.global.mic_gain;
  xcvr_settings.tx_modulation = settings.global.tx_modulation;
  xcvr_settings.pwm_min = settings.global.pwm_min;
  xcvr_settings.pwm_max = settings.global.pwm_max;
  xcvr_settings.pwm_threshold = settings.global.pwm_threshold;
  xcvr_settings.tx_phase_dither = settings.global.tx_phase_dither;
  xcvr_settings.tx_i_offset = settings.global.tx_i_offset;
  xcvr_settings.tx_q_offset = settings.global.tx_q_offset;
  xcvr_settings.tx_iq_balance = settings.global.tx_iq_balance;
  xcvr_settings.tx_use_best_clock = settings.global.tx_use_best_clock;
  xcvr_settings.tx_monitor = settings.global.tx_monitor;
  xcvr_settings.tx_speech_processor = settings.global.tx_speech_processor;
  xcvr_settings.tx_band_limits = settings.global.tx_band_limits;


  transceiver.release();
}

s_memory_channel get_channel(uint16_t channel_number)
{
  static_assert(sizeof(s_memory_channel) < memory_chan_size*4);
  s_memory_channel memory_channel;
  memcpy(&memory_channel.channel, radio_memory[channel_number], sizeof(s_memory_channel));
  memory_channel.label[16] = 0; //add null terminator
  return memory_channel;
}

void memory_store_channel(s_memory_channel memory_channel, uint16_t channel_number, s_settings & settings, xcvr & transceiver, xcvr_settings & xcvr_settings)
{
  static_assert(sizeof(s_memory_channel) < memory_chan_size*4);

  //work out which flash sector the channel sits in.
  const uint32_t num_channels_per_sector = FLASH_SECTOR_SIZE/(sizeof(int)*memory_chan_size);
  const uint32_t first_channel_in_sector = num_channels_per_sector * (channel_number/num_channels_per_sector);
  const uint32_t channel_offset_in_sector = channel_number%num_channels_per_sector;

  //copy sector to RAM
  static uint32_t sector_copy[num_channels_per_sector][memory_chan_size];
  for(uint16_t channel=0; channel<num_channels_per_sector; channel++)
  {
    for(uint16_t location=0; location<memory_chan_size; location++)
    {
      if(channel+first_channel_in_sector < num_chans)
      {
        sector_copy[channel][location] = radio_memory[channel+first_channel_in_sector][location];
      }
    }
  }

  //update the relevant part of the sector
  memcpy(sector_copy[channel_offset_in_sector], &memory_channel, sizeof(s_memory_channel));

  //write sector to flash
  const uint32_t address = (uint32_t)&(radio_memory[first_channel_in_sector]);
  const uint32_t flash_address = address - XIP_BASE;

  //!!! PICO is **very** fussy about flash erasing, there must be no code running in flash.  !!!
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  apply_settings_to_xcvr(transceiver, xcvr_settings, settings, true, false); //suspend xcvr to disable all DMA transfers
  sleep_us(10000);                                    //wait for suspension to take effect
  multicore_lockout_start_blocking();                  //halt the second core
  const uint32_t ints = save_and_disable_interrupts(); //disable all interrupts

  //safe to erase flash here
  //--------------------------------------------------------------------------------------------
  flash_range_erase(flash_address, FLASH_SECTOR_SIZE);
  flash_range_program(flash_address, (const uint8_t*)&sector_copy, FLASH_SECTOR_SIZE);
  //--------------------------------------------------------------------------------------------

  restore_interrupts (ints);                           //restore interrupts
  multicore_lockout_end_blocking();                    //restart the second core
  apply_settings_to_xcvr(transceiver, xcvr_settings, settings, false, false); //resume xcvr operation
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //!!! Normal operation resumed

}


void autosave_restore_settings(s_settings &settings)
{

  //make sure that the memory channels are large enough to store the struct
  static_assert(sizeof(s_settings) < autosave_chan_size*4);

  uint16_t latest_channel = 0xffff;
  for(uint16_t i=0; i<512; i++)
  {
    if(autosave_memory[i][0] != 0xffffffff) //find stored settings
    {
      latest_channel = i;
    }
  }

  if(latest_channel == 0xffff)
  {
    settings = default_settings;
  }
  else
  {
    memcpy(&settings, autosave_memory[latest_channel], sizeof(s_settings));
  }

}

void autosave_store_settings(s_settings settings, xcvr & transceiver, xcvr_settings & xcvr_settings)
{

  //make sure that the memory channels are large enough to store the struct
  static_assert(sizeof(s_settings) < autosave_chan_size*4);

  //copy settings to an array
  uint32_t data_to_save[autosave_chan_size] = {0xffffffff};
  memcpy(data_to_save, &settings, sizeof(s_settings));

  //The flash endurance may not be more than 100000 erase cycles.
  //Cycle through 512 locations, only erasing the flash when they have all been used.
  //This should give an endurance of 51,200,000 cycles.

  //find the next unused channel, an unused channel will be 0xffffffff
  uint16_t empty_channel = 0;
  bool empty_channel_found = false;
  for(uint16_t i=0; i<512; i++)
  {
    volatile uint32_t * check_address = (uint32_t*)&autosave_memory[i][0];
    if(*check_address == 0xffffffff)
    {
      empty_channel = i;
      empty_channel_found = true;
      break;
    }
  }

  //if there are no free channels, erase all the pages
  if(!empty_channel_found)
  {
    const uint32_t address = (uint32_t)&(autosave_memory[0]);
    const uint32_t flash_address = address - XIP_BASE;
    //!!! PICO is **very** fussy about flash erasing, there must be no code running in flash.  !!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    apply_settings_to_xcvr(transceiver, xcvr_settings, settings, true, false); //suspend xcvr to disable all DMA transfers
    sleep_us(10000);                                    //wait for suspension to take effect
    multicore_lockout_start_blocking();                  //halt the second core
    const uint32_t ints = save_and_disable_interrupts(); //disable all interrupts

    //safe to erase flash here
    //--------------------------------------------------------------------------------------------
    flash_range_erase(flash_address, sizeof(int)*autosave_chan_size*512);
    //--------------------------------------------------------------------------------------------

    restore_interrupts (ints);                           //restore interrupts
    multicore_lockout_end_blocking();                    //restart the second core
    apply_settings_to_xcvr(transceiver, xcvr_settings, settings, false, false); //resume xcvr operation
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!! Normal operation resumed
  }

  //work out which flash sector the channel sits in.
  const uint32_t num_channels_per_sector = FLASH_SECTOR_SIZE/(sizeof(int)*autosave_chan_size);
  const uint32_t first_channel_in_sector = num_channels_per_sector * (empty_channel/num_channels_per_sector);
  const uint32_t channel_offset_in_sector = empty_channel%num_channels_per_sector;

  //copy sector to RAM
  static uint32_t sector_copy[num_channels_per_sector][autosave_chan_size];
  for(uint16_t channel=0; channel<num_channels_per_sector; channel++)
  {
    for(uint16_t location=0; location<autosave_chan_size; location++)
    {
      if(channel+first_channel_in_sector < num_chans)
      {
        sector_copy[channel][location] = autosave_memory[channel+first_channel_in_sector][location];
      }
    }
  }

  //modify the selected channel
  for(uint8_t i=0; i<autosave_chan_size; i++)
  {
    sector_copy[channel_offset_in_sector][i] = data_to_save[i];
  }

  //write sector to flash
  const uint32_t address = (uint32_t)&(autosave_memory[first_channel_in_sector]);
  const uint32_t flash_address = address - XIP_BASE;

  //!!! PICO is **very** fussy about flash erasing, there must be no code running in flash.  !!!
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  apply_settings_to_xcvr(transceiver, xcvr_settings, settings, true, false); //suspend xcvr to disable all DMA transfers
  sleep_us(10000);                                    //wait for suspension to take effect
  multicore_lockout_start_blocking();                  //halt the second core
  const uint32_t ints = save_and_disable_interrupts(); //disable all interrupts

  //safe to erase flash here
  //--------------------------------------------------------------------------------------------
  flash_range_program(flash_address, (const uint8_t*)&sector_copy, FLASH_SECTOR_SIZE);
  //--------------------------------------------------------------------------------------------

  restore_interrupts (ints);                           //restore interrupts
  multicore_lockout_end_blocking();                    //restart the second core
  apply_settings_to_xcvr(transceiver, xcvr_settings, settings, false, false);  //resume xcvr operation
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //!!! Normal operation resumed

}
