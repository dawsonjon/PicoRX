#ifndef TX__
#define TX__

#include <stdio.h>
#include <math.h>
#include <ctime>
#include "pico/stdlib.h"
#include "rx.h"
#include "settings.h"

class transmitter
{
  private:

  public:
  rx &receiver;
  rx_settings &settings_to_apply;
  transmitter(rx &receiver, rx_settings &settings_to_apply);
  void set_suspend(bool new_suspend_value, rx &receiver);
  void run_transmitter();

};

#endif
