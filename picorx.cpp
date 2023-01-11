/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// Our assembled program:
#include "nco.pio.h"
#include "rx_dsp.h"

static const uint16_t block_size = 4096;
int16_t ping_audio[block_size];
int16_t pong_audio[block_size];
uint16_t ping_num_audio_samples;
uint16_t pong_num_audio_samples;
bool ping = true;

int main() {

    // Choose which PIO instance to use (there are two instances)
    PIO pio = pio0;

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember this location!
    uint offset = pio_add_program(pio, &hello_program);

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    uint sm = pio_claim_unused_sm(pio, true);
    double tuned_frequency = 1.215e6;
    double actual_frequency = nco_program_init(pio, sm, offset, tuned_frequency);
    stdio_init_all();
    
    //ADC Configuration
    adc_init();
    adc_gpio_init(26);//I channel (0) - configure pin for ADC use
    adc_gpio_init(27);//Q channel (1) - configure pin for ADC use
    adc_set_clkdiv(0); //flat out
    adc_set_round_robin(3); //sample I/Q alternately
    adc_fifo_setup(true, true, 1, false, false);
    
    // Configure DMA for ADC transfers
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);// Pace transfers based on availability of ADC samples

    //Arrays to hold blocks of data
    uint16_t ping_samples[block_size];
    uint16_t pong_samples[block_size];

    //receiver
    rx_dsp rxdsp(tuned_frequency-actual_frequency);

    //bool ping=true;
    adc_select_input(0);
    adc_run(true);
    dma_channel_configure(dma_chan, &cfg, ping_samples, &adc_hw->fifo, block_size, true);

    while (true) 
    {

        dma_channel_wait_for_finish_blocking(dma_chan);
        adc_run(false);
        adc_fifo_drain();
        if (ping)
        {
          //ping buffer is full, start filling pong, and process ping
          adc_select_input(0);
          adc_run(true);
          dma_channel_configure(dma_chan, &cfg, pong_samples, &adc_hw->fifo, block_size, true);
          ping_num_audio_samples = rxdsp.process_block(ping_samples, ping_audio);
          fwrite(ping_audio, 2, ping_num_audio_samples, stdout);
          ping=false;
        }
        else
        {
          //pong buffer is full, start filling ping, and process pong
          adc_select_input(0);
          adc_run(true);
          dma_channel_configure(dma_chan, &cfg, ping_samples, &adc_hw->fifo, block_size, true);
          pong_num_audio_samples = rxdsp.process_block(pong_samples, pong_audio);
          fwrite(pong_audio, 2, pong_num_audio_samples, stdout);
          ping=true;
        }



    }
}
