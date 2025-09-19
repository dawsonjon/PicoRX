//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2023
// filename: transmitter.cpp
// description: Ham Transmitter for Pi Pico
// License: MIT
//


#include "transmit.h"
#include "adc.h"
#include "modulator.h"
#include "nco.h"
#include "pwm.h"
#include "signal_generator.h"

void transmit(tx_mode_t mode, double frequency_Hz,
                       const bool enable_serial_data) {
  const uint8_t mic_pin = 28;
  const uint8_t magnitude_pin = 6;
  const uint8_t rf_pin = 8;
  const double clock_frequency_Hz = rp2350.f_cpu();

  // Use ADC to capture MIC input
  adc mic_adc(mic_pin, 2);

  // Use PWM to output magnitude
  pwm magnitude_pwm(magnitude_pin);

  // Use PIO to output phase/frequency controlled oscillator
  nco rf_nco(rf_pin, clock_frequency_Hz, frequency_Hz);
  const double sample_frequency_Hz = (mode == AM)    ? 12e3
                                     : (mode == FM)  ? 15e3
                                     : (mode == LSB) ? 10e3
                                                     : 10e3;
  const uint8_t waveforms_per_sample =
      rf_nco.get_waveforms_per_sample(clock_frequency_Hz, sample_frequency_Hz);

  // create modulator
  modulator audio_modulator;

  // scale FM deviation
  const double fm_deviation_Hz = 2.5e3; // 2.5kHz
  const uint32_t fm_deviation_f15 =
      round(2 * 32768.0 * fm_deviation_Hz /
            rf_nco.get_sample_frequency_Hz(clock_frequency_Hz, waveforms_per_sample));

  int32_t audio;
  uint16_t magnitude;
  int16_t phase;
  int16_t i; // not used in this design
  int16_t q; // not used in this design

  // use gpio for debug
  uint8_t debug_pin = 1;
  gpio_init(debug_pin);
  gpio_set_dir(debug_pin, GPIO_OUT);
  uint8_t debug_pin_2 = 2;
  gpio_init(debug_pin_2);
  gpio_set_dir(debug_pin_2, GPIO_OUT);

  while (1) {
    // get a sample to transmit
    if (enable_serial_data) {

      // timeout ends transmission
      if (!Serial.available()) return;
      audio = Serial.read();
      
      // read audio from serial port
      audio <<= 8;

    } else {

      // read audio from mic
      audio = mic_adc.get_sample() * 16; // multiply by a gain value
    }

    // demodulate
    gpio_put(debug_pin, 1);
    audio_modulator.process_sample(mode, audio, i, q, magnitude, phase,
                                   fm_deviation_f15);
    gpio_put(debug_pin, 0);

    // output magnitude
    magnitude_pwm.output_sample(magnitude);

    // output phase
    rf_nco.output_sample(phase, waveforms_per_sample);
  }
}
