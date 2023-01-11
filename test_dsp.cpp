#include "rx_dsp.h"
#include <cstdio>

int main()
{
  rx_dsp rx_dsp(10e3);

  uint16_t samples[rx_dsp::block_size];

  int16_t i[rx_dsp::block_size];
  int16_t q[rx_dsp::block_size];

  for(uint16_t idx=0; idx<rx_dsp::block_size; idx++)
  {
      unsigned int sample;
      scanf("%x", &sample);
      samples[idx] = sample;
  }

  rx_dsp.process_block(samples, i, q);
  const uint16_t num_output_samples = rx_dsp.process_block(samples, i, q);

  for(uint16_t idx=0; idx<num_output_samples; idx++)
  {
      if(q[idx] < 0){
        printf("%0i%0ij, ", i[idx], q[idx]);
      }
      else{
        printf("%0i+%0ij, ", i[idx], q[idx]);
      }
  }

}
