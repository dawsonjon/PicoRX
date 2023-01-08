#include <stdint.h>
class RXDSP
{
  public:
  RXDSP(double offset_frequency);
  void process_block(uint16_t samples[], int16_t i_samples[], int16_t q_samples[]);
  static const uint16_t block_size = 1000;
  static const uint8_t stages = 5;

  private:
  int32_t dc;
  uint32_t phase;
  uint32_t frequency;
  uint8_t decimate;
  int16_t sin_table[1024];
  int16_t cos_table[1024];

  int32_t integratori[stages+1], combi[stages+1], delayi[stages];
  int32_t integratorq[stages+1], combq[stages+1], delayq[stages];

};
