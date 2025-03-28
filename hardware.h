#ifndef __hardware_h__
#define __hardware_h__

#define PSU_PIN 23
#define AUDIO_PIN 16

#define PIN_NCO_I 0
#define PIN_NCO_Q 1
// todo - nco.pio
// quadrature_encoder.pio

#define PIN_ADC_I 26
#define PIN_ADC_Q 27
#define PIN_ADC_BATT 29
// todo - adc_select_input calls
// battery_check.cpp
// rx.cpp

#define PIN_VBUS 24

#define PIN_BAND_A 2
#define PIN_BAND_B 3
#define PIN_BAND_C 4

#define PIN_AB 20
#define PIN_B  21
#define PIN_MENU 22
#define PIN_BACK 17
#define PIN_ENCODER_PUSH 5

#define I2C_SDA_PIN 18
#define I2C_SCL_PIN 19
#define I2C_INST (i2c1)
#define I2C_SPEED (400UL)

#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  14
#define PIN_MOSI 15 
#define PIN_DC   11
#define PIN_RST  10
#define SPI_PORT spi1
 

#endif
