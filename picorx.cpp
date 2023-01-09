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
    
    //ADC Conficuration
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
    static const uint16_t block_size = 1000;
    uint16_t samples[block_size];
    int16_t i_samples[block_size];
    int16_t q_samples[block_size];

    //receiver
    rx_dsp rxdsp(tuned_frequency-actual_frequency);

    while (true) 
    {

        
        //DMA 500 samples
        printf("tuned frequency %f actual frequency %f signal offset %f \n", tuned_frequency, actual_frequency, tuned_frequency-actual_frequency);
        clock_t start_time = time_us_64();
        adc_select_input(0);
        adc_run(true);
        dma_channel_configure(dma_chan, &cfg,
            samples,        // dst
            &adc_hw->fifo,  // src
            block_size,            // transfer count
            true            // start immediately
        );
        dma_channel_wait_for_finish_blocking(dma_chan);
        adc_run(false);
        adc_fifo_drain();
        clock_t stop_time = time_us_64();
        printf("time to read samples %f\n", (double)(stop_time - start_time));

        //output samples
        //printf("raw samples\n");
        //stop_time = time_us_64();
        //for(uint16_t idx=0; idx<block_size; idx++)
        //{
            //printf("%04x ", samples[idx]);
            //if((idx+1)%20 == 0) printf("\n");
        //}
        //stop_time = time_us_64();
        //printf("time to dump raw samples %f\n", (double)(stop_time - start_time));

        //Convert Samples to Audio
        start_time = time_us_64();
        const uint16_t num_audio_samples = rxdsp.process_block(samples, i_samples, q_samples);
        stop_time = time_us_64();
        printf("time to frequency shift samples %f\n", (double)(stop_time - start_time));

        //output samples
        printf("procesed sampels:\n");
        stop_time = time_us_64();
        for(uint16_t idx=0; idx<num_audio_samples; idx++)
        {
            printf("%04i ", i_samples[idx]);
            //printf("%04i ", q_samples[idx]);
            if((idx+1)%20 == 0) printf("\n");
        }
        stop_time = time_us_64();
        printf("time to plot samples %f\n", (double)(stop_time - start_time));



    }
}
