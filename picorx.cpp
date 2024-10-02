#include "pico/stdlib.h"
#include <stdio.h>

#include "pico/multicore.h"
#include "pico/time.h"

#include "rx.h"
#include "ui.h"
#include "event.h"
#include "debouncer.h"

#define UI_REFRESH_HZ (10UL)

static rx_settings settings_to_apply;
static rx_status status;
static rx receiver(settings_to_apply, status);
static ui user_interface(settings_to_apply, status,  receiver);

const uint32_t ev_ui_evset = (1UL << ev_tick) |
                             (1UL << ev_button_menu_press) |
                             (1UL << ev_button_menu_release) |
                             (1UL << ev_button_back_press) |
                             (1UL << ev_button_back_release) |
                             (1UL << ev_button_push_press) |
                             (1UL << ev_button_push_release);

static debouncer_t button_menu_deb = {
    .ev_press = {.tag = ev_button_menu_press},
    .ev_release = {.tag = ev_button_menu_release},
    .gpio_num = PIN_MENU,
};

static debouncer_t button_back_deb = {
    .ev_press = {.tag = ev_button_back_press},
    .ev_release = {.tag = ev_button_back_release},
    .gpio_num = PIN_BACK,
};

static debouncer_t button_push_deb = {
    .ev_press = {.tag = ev_button_push_press},
    .ev_release = {.tag = ev_button_push_release},
    .gpio_num = PIN_ENCODER_PUSH,
};

void core1_main()
{
    multicore_lockout_victim_init();
    receiver.run();
}

static bool io_callback(repeating_timer_t *rt)
{
    static uint8_t tick_div = 0;

    debouncer_update(&button_menu_deb);
    debouncer_update(&button_back_deb);
    debouncer_update(&button_push_deb);

    if (++tick_div == (DEBOUNCE_TICK_HZ / UI_REFRESH_HZ))
    {
      tick_div = 0;
      event_t ev = {.tag = ev_tick};
      event_send(ev);
    }

    return true; // keep repeating
}

int main() 
{
  repeating_timer_t io_timer;

  stdio_init_all();
  multicore_launch_core1(core1_main);

  event_init();
  debouncer_init(&button_menu_deb);
  debouncer_init(&button_back_deb);
  debouncer_init(&button_push_deb);  

  //sleep_us(5000000);
  user_interface.autorestore();

  bool ret = add_repeating_timer_us(-1000000 / DEBOUNCE_TICK_HZ, io_callback, NULL, &io_timer);
  hard_assert(ret);

  while(1)
  {
    event_t event = event_get();
    if((1UL << event.tag) & (ev_ui_evset))
    {
      user_interface.do_ui(event);
    }
  }
}
