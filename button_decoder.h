#ifndef __BUTTON_DECODER_H__
#define __BUTTON_DECODER_H__

#include <stdio.h>
#include <stdbool.h>

#include "event.h"

#define BUTTON_DECODER_TICK_HZ (100L)

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        event_t short_press;
        const event_t long_press;
        const event_t long_release;
        bool active;
        int32_t count;
    } button_decoder_t;

    void button_decoder_update(button_decoder_t *dec, bool input);
    void button_decoder_tick(button_decoder_t *dec);
    void button_decoder_other_ev(button_decoder_t *dec);

#ifdef __cplusplus
}
#endif

#endif // __BUTTON_DECODER_H__

