#include "button_decoder.h"

#define _TIMEOUT1_MS (250UL)
#define _TIMEOUT1_TICK (_TIMEOUT1_MS / (1000UL / BUTTON_DECODER_TICK_HZ))

#define _TIMEOUT2_MS (150UL)
#define _TIMEOUT2_TICK (_TIMEOUT2_MS / (1000UL / BUTTON_DECODER_TICK_HZ))

void button_decoder_update(button_decoder_t *dec, bool input)
{
    if (dec->active)
    {
        if (!input)
        {
            if (dec->count > 0)
            {
                dec->short_press.short_press.count++;
                dec->count = _TIMEOUT2_TICK;
            }
            else
            {
                // long release
                event_send(dec->long_release);
                dec->active = false;
                dec->short_press.short_press.count = 0;
            }
        }
        else
        {
            if (dec->count > 0)
            {
                dec->count = _TIMEOUT2_TICK;
            }
        }
    }
    else
    {
        if (input)
        {
            dec->active = true;
            dec->count = _TIMEOUT1_TICK;
        }
    }
}

void button_decoder_tick(button_decoder_t *dec)
{
    if (dec->active)
    {
        if (dec->count > 0)
        {
            dec->count--;
        }

        if(dec->count == 0)
        {
            dec->count--;
            if (dec->short_press.short_press.count == 0)
            {
                // we have a long_press
                event_send(dec->long_press);
            }
            else
            {
                // we have a short press
                event_send(dec->short_press);
                dec->active = false;
                dec->short_press.short_press.count = 0;
            }
        }
    }
}

void button_decoder_other_ev(button_decoder_t *dec)
{
    if (dec->count > 0)
    {
        if (dec->short_press.short_press.count == 0)
        {
            event_send(dec->long_press);
            dec->count = -1;
        }
    }
}
