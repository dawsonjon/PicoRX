#ifndef __EVENT_H__
#define __EVENT_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        ev_none = 0,
        ev_tick,
        ev_button_menu_short_press,
        ev_button_menu_long_press,
        ev_button_menu_long_release,
        ev_button_back_short_press,
        ev_button_back_long_press,
        ev_button_back_long_release,
        ev_button_push_short_press,
        ev_button_push_long_press,
        ev_button_push_long_release,
        ev_encoder_change,
        ev_last,
    } event_e;

    typedef struct
    {
        event_e tag;
        union
        {
            struct
            {
                size_t count;
            } short_press;
            struct
            {
                int32_t new_position;
            } encoder_change;
        };
    } event_t;

    static const char *const event_to_str[] = {
        "ev_none",
        "ev_tick",
        "ev_button_menu_short_press",
        "ev_button_menu_long_press",
        "ev_button_menu_long_release",
        "ev_button_back_short_press",
        "ev_button_back_long_press",
        "ev_button_back_long_release",
        "ev_button_push_short_press",
        "ev_button_push_long_press",
        "ev_button_push_long_release",
        "ev_encoder_change",
    };

    void event_init(void);
    void event_send(event_t event);
    event_t event_get(void);
    void event_print(event_t const *const ev);

#ifdef __cplusplus
}
#endif

#endif // __EVENT_H__
