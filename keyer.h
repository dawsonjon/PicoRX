#ifndef KEYER_H
#define KEYER_H

#include "pico/stdlib.h"
#include "rx.h"
#include "settings.h"

// Keyer state


void setup_paddles();
void process_paddles(rx &receiver, rx_settings &settings);
void add_symbol_to_log(char symbol);
void play_tone_nonblocking(uint32_t frequency, uint32_t duration_ms, uint32_t sample_rate);

#endif
