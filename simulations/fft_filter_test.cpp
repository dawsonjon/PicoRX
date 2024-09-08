#include "../fft_filter.h"
#include <cstdio>
#include <cmath>

int main()
{

  fft_filter filt;

  for(uint8_t j=0; j<4; ++j)
  {
    int16_t i[128] = {0};
    int16_t q[128] = {0};
    uint32_t t = 0;
    for(uint16_t idx = 0; idx<128; ++idx)
    {
      i[idx] = cos(8*2.0*M_PI*t/128.0)*2048*2;// + cos(10*2.0*M_PI*t/128.0)*2048;
      q[idx] = 0; //sin(10*2.0*M_PI*t/2048.0) * 500;
      t++;
    }

    s_filter_control fc;
    fc.start_bin = 0;
    fc.stop_bin = 32;
    fc.upper_sideband = true;
    fc.lower_sideband = true;
    fc.capture = false;

    filt.process_sample(i, q, fc, NULL, NULL);

    for(uint16_t idx = 0; idx<64; ++idx)
    {
      printf("%u %i %i\n", idx, i[idx], q[idx]);
    }
  }

}
