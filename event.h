#ifndef __EVENT_H__
#define __EVENT_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        ev_none = 0,
        ev_tick,
        ev_button_menu_press,
        ev_button_menu_release,
        ev_button_back_press,
        ev_button_back_release,
        ev_button_push_press,
        ev_button_push_release,
        ev_last,
    } event_e;

    typedef struct
    {
        event_e tag;
    } event_t;

    static const char *const event_to_str[] = {
        "ev_none",
        "ev_tick",
        "ev_button_menu_press",
        "ev_button_menu_release",
        "ev_button_back_press",
        "ev_button_back_release",
        "ev_button_push_press",
        "ev_button_push_release",
    };

    void event_init(void);
    void event_send(event_t event);
    event_t event_get(void);
    bool event_has(void);
    void event_print(event_t const *const ev);

#ifdef __cplusplus
}
#endif

#endif // __EVENT_H__
