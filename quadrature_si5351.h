
#include <cstdint>
#include "hardware/i2c.h"

#define SI_OUPUT_ENABLE 3
#define SI_CLK0_CONTROL	16			// Register definitions
#define SI_CLK1_CONTROL	17
#define SI_CLK2_CONTROL	18
#define SI_CLK_DIS_STATE 24
#define SI_SYNTH_PLL_A	26
#define SI_SYNTH_PLL_B	34
#define SI_SYNTH_MS_0		42
#define SI_SYNTH_MS_1		50
#define SI_SYNTH_MS_2		58
#define SI_PLL_RESET		177

#define SI_R_DIV_1		0b00000000			// R-division ratio definitions
#define SI_R_DIV_32		0b01010000

#define SI_CLK_SRC_PLL_A	0b00000000
#define SI_CLK_SRC_PLL_B	0b00100000

const bool low_mode = false;
const bool high_mode = true;

class quad_si5351
{

  void configure_pll(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom);
  void configure_multisynth(uint8_t synth, uint32_t divider, uint8_t rDiv);
  void configure_phase_offset(uint8_t clk, uint8_t phase_ofset);
  void adjust_phase(uint8_t synth, uint32_t a, uint32_t b, uint32_t c, uint8_t rDiv, uint32_t delay_us);
  double set_frequency_hz_high(uint32_t frequency);
  double set_frequency_hz_low(uint32_t frequency);

  i2c_inst_t *m_i2c;
  uint8_t m_address;
  uint32_t m_crystal_frequency_Hz;
  uint32_t m_frequency = 0ull;
  uint32_t m_pll = 0ull;
  uint32_t m_divider = 0ull;
  uint32_t m_rdiv = 0ull;
  bool m_pll_needs_reset = true;
  bool m_mode = low_mode;
  uint8_t m_drive_strength;
   

  public:
  bool initialise(i2c_inst_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint8_t address, uint32_t crystal_frequency_hz);
  void write_reg(uint8_t address, uint8_t data);

  double set_frequency_hz(uint32_t frequency);
  void start();
  void stop();
  void set_drive(uint8_t drive_strength);

  void crystal_load(uint8_t load);

};
