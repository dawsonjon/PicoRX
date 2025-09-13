#include "cic_corrections.h"

#include <cstdint>
#include <algorithm>

#include "rx_definitions.h"

const uint16_t cic_correction[fft_size / 2 + 1] = {
    256,  256,  256,  256,  256,  257,  257,  257,  258,  258,  259,  259,
    260,  260,  261,  262,  263,  264,  264,  265,  266,  268,  269,  270,
    271,  273,  274,  275,  277,  279,  280,  282,  284,  286,  288,  290,
    292,  294,  296,  298,  301,  303,  306,  309,  311,  314,  317,  320,
    323,  326,  330,  333,  337,  340,  344,  348,  352,  356,  361,  365,
    369,  374,  379,  384,  389,  394,  400,  405,  411,  417,  423,  430,
    436,  443,  450,  457,  464,  472,  480,  488,  496,  505,  514,  523,
    533,  542,  553,  563,  574,  585,  597,  608,  621,  634,  647,  660,
    674,  689,  704,  720,  736,  753,  770,  788,  807,  826,  846,  867,
    888,  910,  934,  958,  982,  1008, 1035, 1063, 1092, 1122, 1154, 1187,
    1220, 1256, 1293, 1331, 1371, 1413, 1456, 1501, 1549};

int16_t cic_correct(int16_t fft_bin, int16_t fft_offset, int16_t sample)
{
  int16_t corrected_fft_bin = (fft_bin + fft_offset);
  if(corrected_fft_bin > 127) corrected_fft_bin -= 256;
  if(corrected_fft_bin < -128) corrected_fft_bin += 256;
  uint16_t unsigned_fft_bin = abs(corrected_fft_bin); 
  int32_t adjusted_sample = ((int32_t)sample * cic_correction[unsigned_fft_bin]) >> 8;
  return std::max(std::min(adjusted_sample, (int32_t)INT16_MAX), (int32_t)INT16_MIN);
}
