#include "debouncer.h"

#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/util/queue.h"

#define DEBOUNCE_TIME_MS (20)
#define DEBOUNCE_COUNT (DEBOUNCE_TIME_MS * DEBOUNCE_TICK_HZ / 1000UL)

extern queue_t core0_evq;

void debouncer_init(debouncer_t *deb)
{
    deb->count = 0;
    gpio_init(deb->gpio_num);
    gpio_set_dir(deb->gpio_num, GPIO_IN);
    gpio_pull_up(deb->gpio_num);
}

int8_t debouncer_update(debouncer_t *deb)
{
    int8_t ret = -1;

    if (gpio_get(deb->gpio_num))
    {
        if (deb->count < DEBOUNCE_COUNT)
        {
            deb->count++;
            if (deb->count == DEBOUNCE_COUNT)
            {
                if (deb->ev_release.tag != ev_none)
                {
                    event_send(deb->ev_release);
                }
                ret = 0;
            }
        }
    }
    else
    {
        if (deb->count > 0)
        {
            deb->count--;
            if (deb->count == 0)
            {
                if (deb->ev_press.tag != ev_none)
                {
                    event_send(deb->ev_press);
                }
                ret = 1;
            }
        }
    }
    return ret;
}
