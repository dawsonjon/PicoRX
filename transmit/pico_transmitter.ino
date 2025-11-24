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

// Transmitter example with serial interface
// Supports serial streaming of audio data using
// transmit.py utility

#define BALANCED //comment out for polar amplifier
#define MICROPHONE //comment out for SERIAL mode

#include "transmit.h"

// example application
void setup() 
{
  Serial.begin(12000000);
}

uint32_t frequency = 14250000;
tx_mode_t mode = USB;
void loop()
{
  
  #ifdef MICROPHONE


  //transmit audio from microphone
  transmit(mode, frequency, false);
  
  
  #else
  
  
  //transmit audio from USB serial port
  Serial.println("Pi Pico Transmitter>");    
  while(!Serial.available()) delay(100);
  int command = Serial.read();
  switch (command) {
    // set frequency
    case 'f':
      frequency = Serial.parseInt();
      Serial.print("Frequency: ");
      Serial.println(frequency);
      break;

    // set mode
    case 'm':
      while(!Serial.available()) delay(100);
      command = Serial.read();
      if (command == 'a') {
        Serial.println("MODE=AM");
        mode = AM;
        break;
      } else if (command == 'f') {
        Serial.println("MODE=FM");
        mode = FM;
        break;
      } else if (command == 'l') {
        Serial.println("MODE=LSB");
        mode = LSB;
        break;
      } else if (command == 'u') {
        Serial.println("MODE=USB");
        mode = USB;
        break;
      }
      break;

    // transmit serial data
    case 's':
      Serial.println("Starting transmitter...");
      transmit(mode, frequency, true);
      Serial.println("Transmitter timed out...");
      break;

    // help
    case '?':
      Serial.println("Serial Interface Help");
      Serial.println("=====================");
      Serial.println("fxxxxxx, set frequency Hz");
      Serial.println("mx, set mode, a=AM, f=FM, l=LSB, u=USB");
      Serial.println("s, transmit serial data - (timeout terminates)");
      Serial.println("?, Help (this message)");
      break;
  }
  Serial.println("Pi Pico Transmitter>");


  #endif
}

