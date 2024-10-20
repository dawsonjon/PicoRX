#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        SQUELCH_UNKNOWN = 0, // unknown/unavailable squelch mode
        SQUELCH_ENABLED,     // squelch enabled but signal not detected
        SQUELCH_RISE,        // signal first hit/exceeded threshold
        SQUELCH_SIGNALHI,    // signal level high (above threshold)
        SQUELCH_FALL,        // signal first dropped below threshold
        SQUELCH_SIGNALLO,    // signal level low (below threshold)
        SQUELCH_TIMEOUT,     // signal level low (below threshold for a certain time)
        SQUELCH_DISABLED,    // squelch not enabled
    } squelch_mode_e;

    typedef struct
    {
        squelch_mode_e mode;
        int16_t threshold;
        unsigned int timeout;
        unsigned int timer;
        int16_t s9_threshold;
    } squelch_t;

void squelch_init(squelch_t *self, unsigned int timeout);
void squelch_enable(squelch_t *self, uint8_t s_level);
void squelch_disable(squelch_t *self);
void squelch_calibrate(squelch_t *self, int16_t s9_threshold);
void squelch_set_timeout(squelch_t *self, unsigned int timeout);
void squelch_update(squelch_t *self, int16_t s);

#ifdef __cplusplus
}
#endif
