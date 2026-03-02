//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: adc.cpp
// description: ADC Interface Ham Transmitter
// License: MIT
//

#include "adc.h"

#include <cstdio>

#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#define ADC_BUF_SIZE 256   // must be power of two

static volatile uint16_t adc_buf[ADC_BUF_SIZE] __attribute__((aligned(512)));
static volatile uint dma_chan;
static volatile uint32_t write_index = 0;


adc::adc(const uint8_t mic_pin, const uint8_t adc_input) {

    adc_init();
    adc_gpio_init(mic_pin);
    adc_select_input(adc_input);

    // FIFO: enable, DMA requests, no shift
    adc_fifo_setup(
        true,   // FIFO enable
        true,   // DMA enable
        1,      // DREQ when >=1 sample
        false,
        false
    );

    // Set ADC rate
    adc_set_clkdiv((48e6/250e3)-1);

    // Claim DMA channel
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers from ADC FIFO
    channel_config_set_dreq(&cfg, DREQ_ADC);

    // Enable ring buffer wrapping
    channel_config_set_ring(&cfg, true, 9);
    // 2^8 = 256 entries → matches ADC_BUF_SIZE
    channel_config_set_chain_to(&cfg, dma_chan);

    dma_channel_configure(
        dma_chan,
        &cfg,
        adc_buf,          // write address
        &adc_hw->fifo,    // read address
        0xffffffff,     // transfer count
        true              // start immediately
    );

    adc_run(true);
}

adc::~adc() {
    // Stop ADC free-running
    adc_run(false);

    // Disable DMA channel
    dma_channel_abort(dma_chan);

    // Release the channel for reuse
    dma_channel_unclaim(dma_chan);

    // Optional: clear the buffer (not strictly necessary)
    for (int i = 0; i < ADC_BUF_SIZE; i++) {
        adc_buf[i] = 0;
    }
}

int16_t __not_in_flash_func(adc::get_sample)() {

    // Determine where DMA is currently writing
    uint32_t dma_pos = ((dma_channel_hw_addr(dma_chan)->write_addr) - (uint32_t)adc_buf) >> 1;
    dma_pos &= (ADC_BUF_SIZE - 1); // wrap to buffer size

    // Grab last 4 samples
    uint32_t sum = 0;
    for(int i=0;i<25;i++) {
        sum += adc_buf[(dma_pos - i) & (ADC_BUF_SIZE - 1)];
    }
    uint32_t avg = sum / 25;

    // DC removal
    dc = dc - (dc >> 10) + avg;
    int16_t sample = avg - (dc >> 10);

    return sample;
}
