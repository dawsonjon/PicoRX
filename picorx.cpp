#include "pico/stdlib.h"
#include <stdio.h>

#include "pico/multicore.h"
#include "pico/time.h"

#include "rx.h"
#include "ui.h"
#include "event.h"
#include "debouncer.h"

#define UI_REFRESH_HZ (10UL)
#define UI_REFRESH_US (1000000UL / UI_REFRESH_HZ)

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
                             (1UL << ev_encoder);

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

static const uint32_t sm = 0;
static const PIO pio = pio1;
static int32_t old_position;

static void core1_main()
{
    multicore_lockout_victim_init();
    receiver.run();
}

static void setup_encoder(void)
{
    gpio_set_function(PIN_AB, GPIO_FUNC_PIO1);
    gpio_set_function(PIN_AB+1, GPIO_FUNC_PIO1);
    uint offset = pio_add_program(pio, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio, sm, offset, PIN_AB, 1000);
    old_position = (quadrature_encoder_get_count(pio, sm) + 2)/4;
}

static bool io_callback(repeating_timer_t *rt)
{
    static uint8_t tick_div = 0;

    debouncer_update(&button_menu_deb);
    debouncer_update(&button_back_deb);
    debouncer_update(&button_push_deb);

    int32_t new_position = -((quadrature_encoder_get_count(pio, sm) + 2) / 4);
    int32_t delta = new_position - old_position;
    old_position = new_position;

    if(delta != 0)
    {
      event_t ev = {.tag = ev_encoder, .encoder = {.delta = delta}};
      event_send(ev);
    }

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
  setup_encoder(); 

  //sleep_us(5000000);
  user_interface.autorestore();

  bool ret = add_repeating_timer_us(-1000000 / DEBOUNCE_TICK_HZ, io_callback, NULL, &io_timer);
  hard_assert(ret);

  while(true)
  {
    user_interface.do_disp();
    
    event_t event = event_get();
    if((1UL << event.tag) & (ev_ui_evset))
    {
      user_interface.do_ui(event);
    }
  }
}
