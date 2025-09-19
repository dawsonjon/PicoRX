#include "keyer.h"
#include "pins.h"
#include "pwm_audio_sink.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <math.h>
#include "keyer.pio.h"

// --- CW tone parameters ---
#define CW_TONE_FREQUENCY 700 // Hz
#define WPM 15
const uint32_t dit_length = 1200 / WPM;
const uint32_t dah_length = 3 * dit_length;


static float phase = 0.0f;
static float phase_inc = 0.0f;
static volatile uint32_t tone_remaining_buffers = 0;
static repeating_timer_t tone_timer;


// --- Tone callback ---
bool tone_timer_callback(repeating_timer_t *t) {
    if (tone_remaining_buffers == 0) {
        cancel_repeating_timer(t);
        return false;
    }

    uint slice = pwm_gpio_to_slice_num(PIN_AUDIO);
    for (int i = 0; i < 256; i++) {
        float sample = sinf(phase);
        pwm_set_chan_level(slice, PWM_CHAN_A, (int)((sample + 1.0f) * 2047));
        phase += phase_inc;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
    }

    tone_remaining_buffers--;
    return true;
}

void play_tone_nonblocking(uint32_t frequency, uint32_t duration_ms, uint32_t sample_rate) {
    phase_inc = 2.0f * M_PI * frequency / sample_rate;
    uint32_t total_buffers = (duration_ms * sample_rate) / (256 * 1000);
    tone_remaining_buffers = total_buffers;
    add_repeating_timer_us(1000000LL * 256 / sample_rate, tone_timer_callback, NULL, &tone_timer);
}

