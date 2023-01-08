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
#include "hello.pio.h"
#include "half_band_filter.h"

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
    hello_program_init(pio, sm, offset);
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

    static const uint16_t block_size = 1000;

    uint16_t idx;
    uint16_t samples[block_size];
    uint16_t i_samples[block_size];
    uint16_t q_samples[block_size];

    //dc removal variables
    int32_t dc = 0;

    //frequency shifter variables
    int16_t sin_table[1024];
    int16_t cos_table[1024];
    uint32_t phase = 0;
    double offset_frequency = 125e3;
    uint32_t frequency = ((double)(1ull<<32)*offset_frequency)/500.0e3;

    //pre-generate sin/cos lookup tables
    float scaling_factor = (1 << 16) - 1;
    for(idx=0; idx<1024; idx++)
    {
        sin_table[idx] = sin(2.0*M_PI*idx/1024.0) * scaling_factor;
        cos_table[idx] = cos(2.0*M_PI*idx/1024.0) * scaling_factor;
    }

    //filter
    uint8_t decimate=0;


    while (true) 
    {

        
        //DMA 500 samples
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
        stop_time = time_us_64();
        for(idx=0; idx<block_size; idx++)
        {
            printf("%04x ", samples[idx]);
            if((idx+1)%20 == 0) printf("\n");
        }
        stop_time = time_us_64();
        printf("time to plot samples %f\n", (double)(stop_time - start_time));

        

        //frequency shift
        start_time = time_us_64();
        for(idx=0; idx<block_size; idx++)
        {

            //convert to signed representation
            const int16_t raw_sample = samples[idx] - (1<<11);

            //remove DC
            dc = raw_sample+(dc - (dc >> 7)); //low pass IIR filter
            const int16_t sample = raw_sample - (dc >> 7);
            
            //Convert alternating I/Q samples to complex samples
            //using 0 for the missing components
            const int32_t i = (idx&1)?0:sample;//even samples contain i data
            const int32_t q = (idx&1)?sample:0;//odd samples contain q data

            //Apply frequency shift (move tuned frequency to DC)         
            const int16_t rotation_i =  cos_table[phase>>22]; //32 - 22 = 10MSBs
            const int16_t rotation_q = -sin_table[phase>>22];
            //printf("rotation %u %u %i %i\n", phase, frequency, rotation_i, rotation_q);
            phase += frequency;
            const int16_t i_shifted = (i * rotation_i - q * rotation_q) >> 16;
            const int16_t q_shifted = (q * rotation_i + i * rotation_q) >> 16;
            //i_samples[idx] = i_shifted;
            //q_samples[idx] = q_shifted;

            //decimate by 4
            sum1i = sum1i+i_shifted;
            sum1q = sum1q+q_shifted;
            //onst int32_t sum2i = sum2i+sum1i;
            //const int32_t sum3i = sum3i+sum2i;
            //const int32_t sum4i = sum4i+sum3i;
            //const int32_t sum1q = sum1q+q_shifted;
            //const int32_t sum2q = sum2q+sum1q;
            //const int32_t sum3q = sum3q+sum2q;
            //const int32_t sum4q = sum4q+sum3q;

            decimate++;
            if(decimate = 3)
            {
                decimate = 0;
                 diff1i = sum1i - sum1i_delayed; sum1i_delayed = sum1i;
                 diff1q = sum1q - sum1q_delayed; sum1q_delayed = sum1q;
            //    diff2i = diff1i - diff1i_delayed; diff1i_delayed = diff1i;
            //    diff3i = diff2i - diff2i_delayed; diff2i_delayed = diff2i;
            //    diff4i = diff3i - diff3i_delayed; diff3i_delayed = diff3i;
                i_samples[idx>>2] = diff1i;
                q_samples[idx>>2] = diff1q;
            }
 

        }
        stop_time = time_us_64();
        printf("time to frequency shift samples %f\n", (double)(stop_time - start_time));

        //output samples
        stop_time = time_us_64();
        for(idx=0; idx<block_size; idx++)
        {
            printf("%04x ", i_samples[idx]);
            printf("%04x ", q_samples[idx]);
            if((idx+1)%20 == 0) printf("\n");
        }
        stop_time = time_us_64();
        printf("time to plot samples %f\n", (double)(stop_time - start_time));



    }
}
