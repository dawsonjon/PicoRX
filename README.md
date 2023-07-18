Pi Pico Rx
==========

A Very Minimal SDR reciever using PI pico.

Based around a "Tayloe" Quadrature Sampling Detector, IQ data is captured using
the internal ADC with a bandwidth of 250kHz. The RPI Pico uses the RP2040 PIO
feature to generate a quadrature oscillator eliminating the need for an
external programable oscillator.

Pi Pico Rx is currently at the experimental prototyping stage.
