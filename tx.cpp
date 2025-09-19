#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "settings.h"
#include "rx_definitions.h"
#include "pwm_audio_sink.h"
#include "settings.h"
#include "rx.h"
#include "pins.h"
#include "keyer.h"
#include "tx.h"
#include "hardware/sync.h"

transmitter::transmitter(rx &receiver, rx_settings &rx_setttings) : receiver(receiver), settings_to_apply(rx_setttings)  {
    //setup_paddles();
}

void transmitter::run_transmitter() {
    //process_paddles(receiver, settings_to_apply);
}