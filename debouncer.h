#ifndef __DEBOUNCER_H__
#define __DEBOUNCER_H__

#include <stdio.h>

#include "event.h"

#define DEBOUNCE_TICK_HZ (400L)

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        const event_t ev_press;
        const event_t ev_release;
        const unsigned int gpio_num;
        size_t count;
    } debouncer_t;

    void debouncer_init(debouncer_t *deb);
    int8_t debouncer_update(debouncer_t *deb);

#ifdef __cplusplus
}
#endif

#endif // __DEBOUNCER_H__
