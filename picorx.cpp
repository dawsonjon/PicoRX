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
#include "hardware/pwm.h"
#include "hardware/dma.h"

// Our assembled program:
#include "nco.pio.h"
#include "rx_dsp.h"

int16_t ping_audio[rx_dsp::block_size];
int16_t pong_audio[rx_dsp::block_size];

int main() {
    const uint PSU_PIN = 23;
    gpio_init(PSU_PIN);
    gpio_set_dir(PSU_PIN, GPIO_OUT);
    gpio_put(PSU_PIN, 1);

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
    double tuned_frequency = 1.2150130e6;
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
    uint adc_dma_ping = dma_claim_unused_channel(true);
    uint adc_dma_pong = dma_claim_unused_channel(true);
    dma_channel_config ping_cfg = dma_channel_get_default_config(adc_dma_ping);
    dma_channel_config pong_cfg = dma_channel_get_default_config(adc_dma_pong);

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

    //Arrays to hold blocks of data
    uint16_t ping_samples[rx_dsp::block_size];
    uint16_t pong_samples[rx_dsp::block_size];

    //receiver
    rx_dsp rxdsp(tuned_frequency-actual_frequency);

    //audio output
    const uint AUDIO_PIN = 16;
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    int audio_pwm_slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.f); //125MHz
    pwm_config_set_wrap(&config, 500); 
    pwm_init(audio_pwm_slice_num, &config, true);

    //configure DMA for audio transfers
    int pwm_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config audio_cfg = dma_channel_get_default_config(pwm_dma_chan);
    channel_config_set_transfer_data_size(&audio_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&audio_cfg, true);
    channel_config_set_write_increment(&audio_cfg, false);
    channel_config_set_dreq(&audio_cfg, DREQ_PWM_WRAP0 + audio_pwm_slice_num);

    bool ping=true;
    adc_select_input(0);
    adc_run(true);
    dma_channel_configure(adc_dma_ping, &ping_cfg, ping_samples, &adc_hw->fifo, rx_dsp::block_size, true);
    dma_channel_configure(adc_dma_pong, &pong_cfg, pong_samples, &adc_hw->fifo, rx_dsp::block_size, false);

    const uint PING_PIN = 14;
    const uint BUSY_PIN = 15;
    gpio_init(PING_PIN);
    gpio_init(BUSY_PIN);
    gpio_set_dir(PING_PIN, GPIO_OUT);
    gpio_set_dir(BUSY_PIN, GPIO_OUT);
    uint16_t num_audio_samples;

    while (true) 
    {
        gpio_put(PING_PIN, dma_channel_is_busy(adc_dma_ping));
        gpio_put(BUSY_PIN, dma_channel_is_busy(adc_dma_pong));

        if (ping)
        {
          //ping buffer is full, start filling pong, and process ping
          dma_channel_wait_for_finish_blocking(adc_dma_ping);
          if(dma_channel_is_busy(adc_dma_pong)) dma_channel_configure(adc_dma_ping, &ping_cfg, ping_samples, &adc_hw->fifo, rx_dsp::block_size, false);
          else{
             puts("error\n");
             adc_run(false);
             adc_fifo_drain();
             adc_select_input(0);
             adc_run(true);
             dma_channel_configure(adc_dma_ping, &ping_cfg, ping_samples, &adc_hw->fifo, rx_dsp::block_size, true);
          }
          num_audio_samples = rxdsp.process_block(ping_samples, ping_audio);
          dma_channel_wait_for_finish_blocking(pwm_dma_chan);
          dma_channel_configure(pwm_dma_chan, &audio_cfg, &pwm_hw->slice[audio_pwm_slice_num].cc, ping_audio, num_audio_samples, true);
          ping=false;
        }
        else
        {
          //pong buffer is full, start filling ping, and process pong
          dma_channel_wait_for_finish_blocking(adc_dma_pong);
          if(dma_channel_is_busy(adc_dma_ping)) dma_channel_configure(adc_dma_pong, &pong_cfg, pong_samples, &adc_hw->fifo, rx_dsp::block_size, false);
          else{
            puts("error\n");
            adc_run(false);
            adc_fifo_drain();
            adc_select_input(0);
            adc_run(true);
            dma_channel_configure(adc_dma_pong, &pong_cfg, pong_samples, &adc_hw->fifo, rx_dsp::block_size, true);
          }
          num_audio_samples = rxdsp.process_block(pong_samples, pong_audio);
          dma_channel_wait_for_finish_blocking(pwm_dma_chan);
          dma_channel_configure(pwm_dma_chan, &audio_cfg, &pwm_hw->slice[audio_pwm_slice_num].cc, pong_audio, num_audio_samples, true);
          ping=true;
        }



    }
}
