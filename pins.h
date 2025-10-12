#ifndef __PINS__
#define __PINS__

// GPIO        0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
// PWM Channel 0A 0B 1A 1B 2A 2B 3A 3B 4A 4B 5A 5B 6A 6B 7A 7B
//
// GPIO        16 17 18 19 20 21 22 23 24 25 26 27 28 29
// PWM Channel 0A 0B 1A 1B 2A 2B 3A 3B 4A 4B 5A 5B 6A 6B


const uint8_t PIN_NCO_1        = 0;
const uint8_t PIN_NCO_2        = 1;
const uint8_t PIN_BAND_0       = 2;
const uint8_t PIN_BAND_1       = 3;
const uint8_t PIN_BAND_2       = 4;
const uint8_t PIN_ENCODER_PUSH = 5;
const uint8_t PIN_PTT          = 6;
const uint8_t PIN_DIT          = 7;
const uint8_t PIN_MISO         = 8;
const uint8_t PIN_DAH          = 9;
const uint8_t PIN_MAGNITUDE    = 10; //shared pin, changes use in polar/rectangular mode
const uint8_t PIN_TX_I         = 10; //PWM channel 5A
const uint8_t PIN_DC           = 11;
const uint8_t PIN_RF           = 12; //must be HSTX capable
const uint8_t PIN_TX_Q         = 12; //PWM channel 6A
const uint8_t PIN_CS           = 13;
const uint8_t PIN_SCK          = 14;
const uint8_t PIN_MOSI         = 15;
const uint8_t PIN_AUDIO        = 16; //PWM channel 0A
const uint8_t PIN_BACK         = 17;
const uint8_t PIN_DISPLAY_SDA  = 18;
const uint8_t PIN_DISPLAY_SCL  = 19;
const uint8_t PIN_AB           = 20;
const uint8_t PIN_B            = 21;
const uint8_t PIN_MENU         = 22;
const uint8_t PIN_PSU          = 23;
const uint8_t PIN_VBUS_SENSE   = 24;
const uint8_t LED              = 25;
const uint8_t PIN_ADC_I        = 26;
const uint8_t PIN_ADC_Q        = 27;
const uint8_t PIN_MIC          = 28;
const uint8_t PIN_BATTERY      = 29;

#endif
