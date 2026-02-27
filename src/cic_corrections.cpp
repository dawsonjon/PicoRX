#include "cic_corrections.h"

#include <cstdint>
#include <algorithm>

#include "rx_definitions.h"

const uint16_t cic_correction[fft_size / 2 + 1] = {
    256,  256,  256,  256,  256,  257,  257,  257,  258,  258,  259,  260,
    260,  261,  262,  263,  264,  265,  266,  267,  268,  270,  271,  273,
    274,  276,  277,  279,  281,  283,  285,  287,  289,  291,  294,  296,
    299,  301,  304,  307,  310,  313,  316,  319,  323,  326,  330,  334,
    337,  341,  346,  350,  354,  359,  364,  369,  374,  379,  384,  390,
    396,  402,  408,  414,  421,  428,  435,  442,  450,  458,  466,  474,
    483,  492,  501,  511,  520,  531,  541,  553,  564,  576,  588,  601,
    614,  628,  642,  657,  672,  688,  705,  722,  739,  758,  777,  797,
    818,  840,  862,  886,  910,  936,  962,  990,  1019, 1049, 1080, 1113,
    1147, 1183, 1220, 1259, 1300, 1343, 1387, 1434, 1483, 1535, 1589, 1645,
    1705, 1767, 1833, 1902, 1974, 2050, 2131, 2215, 2305};

int16_t cic_correct(int16_t fft_bin, int16_t fft_offset, int16_t sample)
{
  int16_t corrected_fft_bin = (fft_bin + fft_offset);
  if(corrected_fft_bin > 127) corrected_fft_bin -= 256;
  if(corrected_fft_bin < -128) corrected_fft_bin += 256;
  uint16_t unsigned_fft_bin = abs(corrected_fft_bin);
  int32_t adjusted_sample = ((int32_t)sample * cic_correction[unsigned_fft_bin]) >> 8;
  return std::max(std::min(adjusted_sample, (int32_t)INT16_MAX), (int32_t)INT16_MIN);
}
