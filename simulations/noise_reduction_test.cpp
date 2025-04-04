#include <cstdint>
#include <algorithm>
#include <iostream>

#include "../noise_reduction.h"


int main()
{

  int16_t i[128];
  int16_t q[128];
  int32_t noise_estimate[128] = {0};
  int16_t signal_estimate[128] = {0};

  for(int idx=0; idx<128; ++idx)
  {
    noise_estimate[idx] = INT32_MAX-1;
    signal_estimate[idx] = 0;
  }

  uint16_t frame = 0;
  while(1)
  {

    for(int idx=0; idx<128; ++idx)
    {
      int i_int, q_int;
      std::cin >> i_int >> q_int;
      i[idx] = i_int;
      q[idx] = q_int;
    }

    std::cerr << "input frame" << frame << std::endl;

    noise_reduction(i, q, noise_estimate, signal_estimate, 0, 127, 11, 4);

    std::cerr << "output frame" << frame++ << std::endl;

    for(int idx=0; idx<128; ++idx)
    {
      std::cout << i[idx] << " " << q[idx] << " " << noise_estimate[idx] << " " << 1024*signal_estimate[idx] << " ";
    }
    std::cout << std::endl;

  }
}
