#include "rotary_encoder.h"

#include "pins.h"
#include "ui.h"

static int32_t count;
static critical_section_t crit;

static void __not_in_flash_func(gpio_callback)(uint gpio, uint32_t events) {
  static const int8_t rot_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1,
                                         1, 0, 0, 1, 0, 1, 1, 0};

  static uint8_t prevNextCode = 0;
  static uint16_t store = 0;

  (void)gpio;
  (void)events;

  prevNextCode <<= 2;
  if (gpio_get(PIN_AB)) prevNextCode |= 0x02;
  if (gpio_get(PIN_B)) prevNextCode |= 0x01;
  prevNextCode &= 0x0f;

  if (rot_enc_table[prevNextCode]) {
    store <<= 4;
    store |= prevNextCode;
    // for less noisy encoders
    // if (store==0xd42b) count--;
    // if (store==0xe817) count++;
    critical_section_enter_blocking(&crit);
    if ((store & 0xff) == 0x2b) count--;
    if ((store & 0xff) == 0x17) count++;
    critical_section_exit(&crit);
  }
}

rotary_encoder::rotary_encoder(s_global_settings& _settings)
    : encoder(_settings) {
  gpio_init(PIN_AB);
  gpio_set_dir(PIN_AB, GPIO_IN);
  gpio_pull_up(PIN_AB);

  gpio_init(PIN_B);
  gpio_set_dir(PIN_B, GPIO_IN);
  gpio_pull_up(PIN_B);

  critical_section_init(&crit);

  gpio_set_irq_callback(gpio_callback);
  gpio_set_irq_enabled(PIN_AB, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(PIN_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  irq_set_enabled(IO_IRQ_BANK0, true);

  old_position = new_position = 0;
}

int32_t rotary_encoder::get_change(void) {
  int32_t c = 0;
  critical_section_enter_blocking(&crit);
  c = count;
  critical_section_exit(&crit);

  if (settings.encoder_resolution) {
    new_position = -c;
  } else {
    new_position = -(c + 1) / 2;
  }

  int32_t delta = new_position - old_position;
  old_position = new_position;
  if (settings.reverse_encoder & 1) {
    return -delta;
  } else {
    return delta;
  }
}
