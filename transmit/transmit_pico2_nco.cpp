//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: transmit_nco.cpp
// description: HSTX based NCO for Pi Pico
// License: MIT
//

#include "transmit_pico2_nco.h"
#include "hardware/structs/hstx_ctrl.h"
#include "hardware/structs/hstx_fifo.h"

void transmit_nco::initialise_waveform_buffer(uint32_t buffer[],
                                     uint32_t waveform_length_words,
                                     double normalised_frequency) {

  SinLut &sin_lut = SinLut::instance();
  uint32_t phase_increment = round(normalised_frequency * 4294967296.0);
  for (uint8_t advance = 0u; advance < bits_per_word; ++advance) {
    uint32_t phase = advance*phase_increment;
    for (uint16_t word = 0; word < waveform_length_words * 2; ++word) {
      uint32_t bit_samples = 0;
      for (uint8_t bit = 0; bit < bits_per_word; ++bit) {
        int16_t sample = sin_lut.waveform[phase>>21]; //shift 21 bits to the right, keeping 11 MSBs
        phase += phase_increment;
        // could apply dithering here to remove harmonics
        if(m_dither)
        {
          sample += (rand() & 0x7fff) - 16383;
        }
        if (sample > 0) {
          bit_samples |= (1 << bit);
        }
      }
      buffer[(advance * waveform_length_words * 2) + word] = bit_samples;
    }
  }
}

transmit_nco::transmit_nco(const uint8_t rf_pin, double clock_frequency_Hz, double frequency_Hz, bool dither) {
  m_rf_pin = rf_pin;
  m_dither = dither;


  //configure hstx
  gpio_set_function(m_rf_pin, (gpio_function_t)0); //gpio_hstx

  hstx_ctrl_hw->csr = 0;
  hstx_ctrl_hw->csr =
      16u << HSTX_CTRL_CSR_N_SHIFTS_LSB | //16 shifts of 2 bits each
      2u << HSTX_CTRL_CSR_SHIFT_LSB | //2 bits each
      HSTX_CTRL_CSR_EN_BITS; //enable

  hstx_ctrl_hw->bit[0] =
      0 << HSTX_CTRL_BIT0_SEL_P_LSB | //shift register bit 0 on first half cycle
      1 << HSTX_CTRL_BIT0_SEL_N_LSB; //shift register bit 1 on second half cycle



  // calculate some constants
  const double normalised_frequency = frequency_Hz / (2*clock_frequency_Hz);
  const double period_clocks = (2*clock_frequency_Hz) / frequency_Hz;

  // Each 512 bit waveform will not necessarily be a whole number of cycles,
  // we will probably be partway through a cycle at the end.
  // This constant tells us how many clocks (and part clocks) we were through
  // the last cycle at the end of a 512-bit waveform. Next time, we choose a
  // waveform that has been advanced by this many clocks so that we pick up
  // where we left off (to the nearest clock). The fraction part is
  // accumulated so that the any rounding error we introduce can be corrected
  // for later. 9 integer bits allow up to 512 whole clocks of advancement,
  // 32 bits are fraction bits.
  index_increment_f32 = round(fmod(waveform_length_bits, period_clocks) *
                              pow(2.0, fraction_bits));
  wrap_f32 = period_clocks * pow(2.0, fraction_bits);

  // we may want to adjust the phase on-the fly.
  // A 16-bit phase adjustment will have steps of period/16384 i.e.
  // -period/16384 +period/16383
  phase_step_clocks_f32 =
      (0.5 * period_clocks / 16384.0) * pow(2.0, 32);

  // store 32 waveforms
  // 32 copies of the waveform are stored, each advanced by 1 clock more than
  // the last advancements of up to 31 bits are achieved by selecting the
  // appropriately advanced waveform. advancements of 32 bits or more can be
  // achieved by advancing whole words through each waveform.
  initialise_waveform_buffer(buffer, waveform_length_words,
                             normalised_frequency);


  // 2 DMA channels are used.
  // 1 transfers the pre-generated blocks of 512-bits to HSTX
  // The second DMA configures the first from a table of start addresses
  nco_dma = dma_claim_unused_channel(true);
  chain_dma = dma_claim_unused_channel(true);

  // configure DMA from memory to HSTX FIFO
  nco_dma_cfg = dma_channel_get_default_config(nco_dma);
  channel_config_set_transfer_data_size(&nco_dma_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&nco_dma_cfg, true);
  channel_config_set_write_increment(&nco_dma_cfg, false);
  channel_config_set_dreq(&nco_dma_cfg, DREQ_HSTX);
  channel_config_set_chain_to(&nco_dma_cfg, chain_dma);
  channel_config_set_irq_quiet(&nco_dma_cfg,
                               true); // don't generate interrupt for each
                                      // transfer, only on NULL transfer
  dma_channel_configure(nco_dma, &nco_dma_cfg, &hstx_fifo_hw->fifo, NULL, waveform_length_words,
                        false); // 8x32bit transfers

  // use a second DMA to reconfigure the first
  chain_dma_cfg = dma_channel_get_default_config(chain_dma);
  channel_config_set_transfer_data_size(&chain_dma_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&chain_dma_cfg,
                                    true); // works through a list of transfers
  channel_config_set_write_increment(
      &chain_dma_cfg, false); // always writes to data DMA read address
  dma_channel_start(nco_dma);

}

transmit_nco::~transmit_nco() {

  dma_channel_cleanup(nco_dma);
  dma_channel_cleanup(chain_dma);
  dma_channel_unclaim(nco_dma);
  dma_channel_unclaim(chain_dma);

  // disable GPIO, pullup/pulldown resistors should be installed
  // to switch off transistors when pin is high impedance
  gpio_deinit(m_rf_pin);
}

// returns the nearest number of waveforms per sample for a given sample
// frequency
uint8_t transmit_nco::get_waveforms_per_sample(double clock_frequency_Hz, double sample_frequency_Hz) {
  return (2*clock_frequency_Hz) / (waveform_length_bits * sample_frequency_Hz);
}

// returns the exact sample frequency for a given number of waveforms per sample
double transmit_nco::get_sample_frequency_Hz(double clock_frequency_Hz, uint8_t waveforms_per_sample) {
  return (2*clock_frequency_Hz) / (waveform_length_bits * waveforms_per_sample);
}

// Run NCO for one audio sample.
//
// phase is a signed 16-bit number representing -pi/2 <= x < pi/2
// waveforms per sample is the number of 512-clock waveforms to output
// this determines the audio sample frequency e.g. 32 gives ~15kHz 100
// gives 4.8kHz

void __not_in_flash_func(transmit_nco::output_sample)(int16_t phase, uint8_t waveforms_per_sample) {

  assert(waveforms_per_sample < max_waveforms_per_sample);

  // null transfer at the end of each 32 address block
  buffer_addresses[ping_pong][waveforms_per_sample] = NULL;

  // plan next 32 transfers while the last 32 are sending
  for (uint8_t transfer = 0u; transfer < waveforms_per_sample; ++transfer) {
    uint64_t phase_modulation_f32 = (phase_step_clocks_f32 * phase);
    int64_t modulated_index_f32 = index_f32 + phase_modulation_f32;
    if (modulated_index_f32 > wrap_f32) {
      modulated_index_f32 -= wrap_f32;
    }
    if (modulated_index_f32 < 0) {
      modulated_index_f32 += wrap_f32;
    }
    uint16_t index = (modulated_index_f32) >> fraction_bits;
    // The lower 5 bits represent up to 31 bits of advance
    // The upper 4 bits represent the number of whole words of advance
    // we need to swap these because the words of each waveform are contiguous
    // keep in mind that the buffer is twice the length of the waveform, so bottom
    // bits need to be shifted up by 5 rather than 4.
    index = ((index & 0x1f) << 5) | ((index & 0x1e0) >> 5);
    buffer_addresses[ping_pong][transfer] = &buffer[index];
    index_f32 += index_increment_f32;
    if (index_f32 > wrap_f32) {
      index_f32 -= wrap_f32;
    }
  }

  // if a dma is running, wait until it completes
  interrupts = save_and_disable_interrupts();
  if (dma_started) {
    while (!(dma_hw->intr & 1u << nco_dma)) {
      tight_loop_contents();
    }
    dma_hw->ints0 = 1u << nco_dma;
  }

  ///////////////////////////////////////////////////////////////////////
  // HSTX FIFO has 8 entries, so need to kick off next DMA within 32 x 8 = 512
  // clocks of the last one finishing
  // don't put anything between here and next DMA
  // don't put new code here!!
  ///////////////////////////////////////////////////////////////////////

  // chain nco DMA
  dma_channel_configure(
      chain_dma, &chain_dma_cfg,
      &dma_hw->ch[nco_dma].al3_read_addr_trig, // overwrite nco_dma read address
                                               // and trigger new transfer
      &buffer_addresses[ping_pong][0],         // increments each time
      1,                                       // Halt after 1 transfer
      true                                     // start now
  );

  restore_interrupts(interrupts);
  ping_pong ^= 1;
  dma_started = true;
}
