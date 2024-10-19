#include "squelch.h"

void squelch_init(squelch_t *self, unsigned int timeout)
{
    squelch_disable(self);
    squelch_set_timeout(self, timeout);
    self->s9_threshold = 0;
}

void squelch_enable(squelch_t *self, uint8_t s_level)
{
    // 0-9 = s0 to s9, 10 to 12 = S9+10dB to S9+30dB
    const int16_t thresholds[] = {
        (int16_t)(self->s9_threshold >> 9), // s0
        (int16_t)(self->s9_threshold >> 8), // s1
        (int16_t)(self->s9_threshold >> 7), // s2
        (int16_t)(self->s9_threshold >> 6), // s3
        (int16_t)(self->s9_threshold >> 5), // s4
        (int16_t)(self->s9_threshold >> 4), // s5
        (int16_t)(self->s9_threshold >> 3), // s6
        (int16_t)(self->s9_threshold >> 2), // s7
        (int16_t)(self->s9_threshold >> 1), // s8
        (int16_t)(self->s9_threshold),      // s9
        (int16_t)(self->s9_threshold * 3),  // s9+10dB
        (int16_t)(self->s9_threshold * 10), // s9+20dB
        (int16_t)(self->s9_threshold * 31), // s9+30dB
    };
    self->threshold = thresholds[s_level];
    self->mode = SQUELCH_ENABLED;
}

void squelch_disable(squelch_t *self)
{
    self->mode = SQUELCH_DISABLED;
}

void squelch_calibrate(squelch_t *self, int16_t s9_threshold)
{
    self->s9_threshold = s9_threshold;
}

void squelch_set_timeout(squelch_t *self, unsigned int timeout)
{
    self->timeout = timeout;
}

void squelch_update(squelch_t *self, int16_t s)
{
    int threshold_exceeded = (s > self->threshold);

    switch (self->mode)
    {
    case SQUELCH_ENABLED:
        self->mode = threshold_exceeded ? SQUELCH_RISE : SQUELCH_ENABLED;
        break;
    case SQUELCH_RISE:
        self->mode = threshold_exceeded ? SQUELCH_SIGNALHI : SQUELCH_FALL;
        break;
    case SQUELCH_SIGNALHI:
        self->mode = threshold_exceeded ? SQUELCH_SIGNALHI : SQUELCH_FALL;
        break;
    case SQUELCH_FALL:
        self->mode = threshold_exceeded ? SQUELCH_SIGNALHI : SQUELCH_SIGNALLO;
        self->timer = self->timeout;
        break;
    case SQUELCH_SIGNALLO:
        self->timer--;
        if (self->timer == 0)
            self->mode = SQUELCH_TIMEOUT;
        else if (threshold_exceeded)
            self->mode = SQUELCH_SIGNALHI;
        break;
    case SQUELCH_TIMEOUT:
        self->mode = SQUELCH_ENABLED;
        break;
    case SQUELCH_DISABLED:
        break;
    case SQUELCH_UNKNOWN:
        break;
    }
}
