#include "event.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"

// the UI code still has some large blocking delays, so give it some slack
#define CORE0_EVQ_LEN (4)

static queue_t core0_evq;

void event_init(void)
{
    queue_init(&core0_evq, sizeof(event_t), CORE0_EVQ_LEN);
}

void event_send(event_t event)
{
    bool s = queue_try_add(&core0_evq, &event);
    hard_assert(s);
}

event_t event_get(void)
{
    event_t event;
    queue_remove_blocking(&core0_evq, &event);
    return event;
}

void event_print(event_t const *const ev)
{
    if (ev->tag < ev_last)
    {
        uint32_t _time_us = time_us_32();
        printf("%ld: Event: %s\n", _time_us, event_to_str[ev->tag]);
    }
}
