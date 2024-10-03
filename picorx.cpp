#include "pico/stdlib.h"
#include <stdio.h>

#include "pico/multicore.h"
#include "pico/time.h"

#include "rx.h"
#include "ui.h"
#include "event.h"
#include "debouncer.h"
#include "button_decoder.h"
#include "quadrature_encoder.pio.h"

#define UI_REFRESH_HZ (10UL)

static rx_settings settings_to_apply;
static rx_status status;
static rx receiver(settings_to_apply, status);
static ui user_interface(settings_to_apply, status,  receiver);

const uint32_t ev_ui_evset = (1UL << ev_tick) |
                             (1UL << ev_button_menu_short_press) |
                             (1UL << ev_button_menu_long_press) |
                             (1UL << ev_button_menu_long_release) |
                             (1UL << ev_button_back_short_press) |
                             (1UL << ev_button_back_long_press) |
                             (1UL << ev_button_back_long_release) |
                             (1UL << ev_button_push_short_press) |
                             (1UL << ev_button_push_long_press) |
                             (1UL << ev_button_push_long_release) |
                             (1UL << ev_encoder_change);

static debouncer_t button_menu_deb = {
    .ev_press = {.tag = ev_none},
    .ev_release = {.tag = ev_none},
    .gpio_num = PIN_MENU,
};

static debouncer_t button_back_deb = {
    .ev_press = {.tag = ev_none},
    .ev_release = {.tag = ev_none},
    .gpio_num = PIN_BACK,
};

static debouncer_t button_push_deb = {
    .ev_press = {.tag = ev_none},
    .ev_release = {.tag = ev_none},
    .gpio_num = PIN_ENCODER_PUSH,
};

static button_decoder_t button_menu_dec = {
    .short_press = {.tag = ev_button_menu_short_press},
    .long_press = {.tag = ev_button_menu_long_press},
    .long_release = {.tag = ev_button_menu_long_release},
    .active = false,
    .count = 0,
};

static button_decoder_t button_back_dec = {
    .short_press = {.tag = ev_button_back_short_press},
    .long_press = {.tag = ev_button_back_long_press},
    .long_release = {.tag = ev_button_back_long_release},
    .active = false,
    .count = 0,
};

static button_decoder_t button_push_dec = {
    .short_press = {.tag = ev_button_push_short_press},
    .long_press = {.tag = ev_button_push_long_press},
    .long_release = {.tag = ev_button_push_long_release},
    .active = false,
    .count = 0,
};

static const uint32_t sm = 0;
static const PIO pio = pio1;
static int32_t old_position;

void core1_main()
{
    multicore_lockout_victim_init();
    receiver.run();
}

static bool io_callback(repeating_timer_t *rt)
{
    static uint8_t tick_div = 0;
    static uint8_t btn_tick_div = 0;

    int8_t ret = debouncer_update(&button_menu_deb);
    if (ret >= 0)
    {
      button_decoder_update(&button_menu_dec, ret == 1);
    }

    ret = debouncer_update(&button_back_deb);
    if (ret >= 0)
    {
      button_decoder_update(&button_back_dec, ret == 1);
    }

    ret = debouncer_update(&button_push_deb);
    if (ret >= 0)
    {
      button_decoder_update(&button_push_dec, ret == 1);
    }

    if (++tick_div == (DEBOUNCE_TICK_HZ / UI_REFRESH_HZ))
    {
      tick_div = 0;

      int32_t new_position = quadrature_encoder_get_count(pio, sm);
      if (new_position != old_position)
      {
        button_decoder_other_ev(&button_menu_dec);
        button_decoder_other_ev(&button_back_dec);
        button_decoder_other_ev(&button_push_dec);
        event_t ev = {.tag = ev_encoder_change, .encoder_change = {.new_position = new_position}};
        event_send(ev);
        old_position = new_position;
      } else {
        event_t ev = {.tag = ev_tick};
        event_send(ev);
      }
    }

    if (++btn_tick_div == (DEBOUNCE_TICK_HZ / BUTTON_DECODER_TICK_HZ))
    {
      btn_tick_div = 0;
      button_decoder_tick(&button_menu_dec);
      button_decoder_tick(&button_back_dec);
      button_decoder_tick(&button_push_dec);
    }

    return true; // keep repeating
}

static void setup_encoder(void)
{
    gpio_set_function(PIN_AB, GPIO_FUNC_PIO1);
    gpio_set_function(PIN_AB+1, GPIO_FUNC_PIO1);
    uint offset = pio_add_program(pio, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio, sm, offset, PIN_AB, 1000);
    old_position = quadrature_encoder_get_count(pio, sm);
}

int main() 
{
  repeating_timer_t io_timer;

  // create an alarm pool for USB streaming with highest priority (0), so
  // that it can pre-empt the default pool used by IO which uses timer 3 (lowest priority)
  receiver.set_alarm_pool(alarm_pool_create(0, 16));
  
  multicore_launch_core1(core1_main);
  stdio_init_all();

  event_init();
  debouncer_init(&button_menu_deb);
  debouncer_init(&button_back_deb);
  debouncer_init(&button_push_deb);
  setup_encoder();

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
