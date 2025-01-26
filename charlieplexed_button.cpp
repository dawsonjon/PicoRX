#include "charlieplexed_button.h"
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include <cstdio>


auto_init_mutex(button_mutex);

charlieplexed_button :: charlieplexed_button(uint8_t gpio_num, uint8_t gpio_enable_num):gpio_num(gpio_num), gpio_enable_num(gpio_enable_num)
{
    button_state = up;
    time_pressed = 0;
    gpio_init(gpio_num);
    gpio_set_dir(gpio_num, GPIO_IN);
    gpio_pull_up(gpio_num);
}

bool __not_in_flash_func(charlieplexed_button) :: is_keyed()
{
  mutex_enter_blocking(&button_mutex);

  gpio_set_dir(gpio_enable_num, GPIO_OUT);
  gpio_put(gpio_enable_num, 0);
  sleep_us(1);

  bool is_keyed = !gpio_get(gpio_num);

  //drive high before disabling is faster
  gpio_put(gpio_enable_num, 1);
  sleep_us(1);
  gpio_set_dir(gpio_enable_num, GPIO_IN);

  mutex_exit(&button_mutex);

  return is_keyed;
}

void charlieplexed_button :: update_state()
{
  mutex_enter_blocking(&button_mutex);

  gpio_put(gpio_enable_num, 0);
  gpio_set_dir(gpio_enable_num, GPIO_OUT);
  sleep_us(10);

  if(button_state == up)
  {
    if(!gpio_get(gpio_num)) //on
    {
      time_pressed = time_us_32();
      button_state = down;
    }
  }
  else if(button_state == down)
  {
    if((time_us_32() - time_pressed) > (500 * 1000))
    {
      button_state = held;
    }
    else if(gpio_get(gpio_num)) //off
    {
      if((time_us_32() - time_pressed) > (50 * 1000))
      {
        button_state = pressed;
      }
      else
      {
        button_state = up;
      }
    }
  }
  else if(button_state == pressed)
  {
    button_state = up;
  }
  else if(button_state == held)
  {
    if(gpio_get(gpio_num)) //off
    {
      button_state = up;
    }
  }

  gpio_set_dir(gpio_enable_num, GPIO_IN);
  gpio_put(gpio_enable_num, 1);

  mutex_exit(&button_mutex);
}

bool charlieplexed_button :: is_pressed()
{
  update_state();
  return button_state == pressed;
}

bool charlieplexed_button :: is_held()
{
  update_state();
  return button_state == held;
}
