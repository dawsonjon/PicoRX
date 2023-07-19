Pi Pico Rx
==========

A Very Minimal SDR reciever using PI pico.



Based around a "Tayloe" Quadrature Sampling Detector, IQ data is captured using
the internal ADC with a bandwidth of 250kHz. The RPI Pico uses the RP2040 PIO
feature to generate a quadrature oscillator eliminating the need for an
external programable oscillator. Audio is generated using a PWM io output.

All this means we can build a useful LW/MW/SF SDR reciever using a [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/), an [analogue switch](https://www.onsemi.com/products/interfaces/analog-switches/fst3253) an [op-amp](https://www.analog.com/media/en/technical-documentation/data-sheets/ltc6226-6227.pdf) and a handful of discrete components!

![concept](images/concept.svg)

Features
--------

+ 0 - 30MHz coverage
+ 250kHz SDR reciever
+ CW/SSB/AM/FM reception
+ OLED display
+ simple spectrum scope
+ Headphones/Speaker
+ 500 general purpose memories

Development
-----------

![concept](images/top.svg)

Pi Pico Rx is currently at the experimental prototyping stage, but it works.... There is a PCB on the way.
