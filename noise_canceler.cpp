#include "noise_canceler.h"

#include <string.h>

#include "fft_filter.h"
#include "rx_definitions.h"

#define MAG_SIZE (new_fft_size)
#define NUM_BINS (10)

#define LAMBDA (1638)         // 0.05
#define FIX_ONE (32767)       // 1.0
#define ALPHA (50 * FIX_ONE)  // 50.0

#define ALPHA_LOW (32767)    // 1
#define ALPHA_HIGH (196602)  // 6

#define SNR_LIN_LOW (18426)    // 0.5623413251903491
#define SNR_LIN_HIGH (327670)  // 10.0
#define SNR_LUT_SCALE (818)

static const uint32_t alpha_lut[] = {
    196602, 194126, 191753, 189476, 187285, 185176, 183143, 181179, 179281,
    177445, 175665, 173940, 172265, 170639, 169057, 167518, 166020, 164560,
    163137, 161748, 160393, 159069, 157775, 156510, 155272, 154061, 152874,
    151713, 150574, 149458, 148363, 147289, 146234, 145199, 144183, 143184,
    142202, 141237, 140289, 139355, 138437, 137534, 136644, 135768, 134906,
    134056, 133219, 132394, 131580, 130779, 129988, 129208, 128439, 127680,
    126931, 126191, 125462, 124741, 124029, 123327, 122632, 121947, 121269,
    120599, 119937, 119283, 118636, 117996, 117364, 116738, 116119, 115507,
    114902, 114303, 113710, 113123, 112542, 111967, 111397, 110834, 110276,
    109723, 109176, 108634, 108097, 107565, 107037, 106515, 105998, 105485,
    104977, 104473, 103973, 103478, 102988, 102501, 102019, 101540, 101066,
    100596, 100129, 99666,  99207,  98752,  98300,  97852,  97407,  96966,
    96528,  96093,  95662,  95234,  94809,  94387,  93969,  93553,  93141,
    92731,  92325,  91921,  91520,  91122,  90727,  90334,  89944,  89557,
    89172,  88790,  88411,  88034,  87659,  87287,  86918,  86550,  86186,
    85823,  85463,  85105,  84749,  84395,  84044,  83695,  83348,  83003,
    82660,  82319,  81980,  81643,  81308,  80975,  80644,  80315,  79988,
    79663,  79340,  79018,  78698,  78380,  78064,  77749,  77437,  77126,
    76816,  76508,  76202,  75898,  75595,  75294,  74994,  74696,  74400,
    74105,  73812,  73520,  73229,  72940,  72653,  72367,  72082,  71799,
    71517,  71237,  70958,  70680,  70404,  70129,  69855,  69583,  69312,
    69042,  68773,  68506,  68240,  67975,  67712,  67449,  67188,  66928,
    66670,  66412,  66156,  65900,  65646,  65393,  65141,  64891,  64641,
    64392,  64145,  63898,  63653,  63409,  63165,  62923,  62682,  62442,
    62203,  61964,  61727,  61491,  61256,  61022,  60788,  60556,  60325,
    60094,  59865,  59636,  59408,  59182,  58956,  58731,  58507,  58283,
    58061,  57839,  57619,  57399,  57180,  56962,  56745,  56528,  56313,
    56098,  55884,  55671,  55458,  55247,  55036,  54826,  54617,  54408,
    54200,  53993,  53787,  53582,  53377,  53173,  52969,  52767,  52565,
    52364,  52163,  51964,  51765,  51566,  51369,  51172,  50975,  50780,
    50585,  50391,  50197,  50004,  49812,  49620,  49429,  49239,  49049,
    48860,  48671,  48483,  48296,  48110,  47924,  47738,  47553,  47369,
    47185,  47002,  46820,  46638,  46457,  46276,  46096,  45916,  45737,
    45559,  45381,  45204,  45027,  44851,  44675,  44500,  44325,  44151,
    43978,  43805,  43632,  43460,  43289,  43118,  42947,  42777,  42608,
    42439,  42271,  42103,  41935,  41768,  41602,  41436,  41271,  41106,
    40941,  40777,  40613,  40450,  40288,  40125,  39964,  39803,  39642,
    39481,  39321,  39162,  39003,  38844,  38686,  38529,  38371,  38215,
    38058,  37902,  37747,  37592,  37437,  37283,  37129,  36975,  36822,
    36670,  36518,  36366,  36214,  36063,  35913,  35763,  35613,  35463,
    35314,  35166,  35018,  34870,  34722,  34575,  34428,  34282,  34136,
    33991,  33845,  33701,  33556,  33412,  33268,  33125,  32982,  32839,
};

const uint16_t update_freq = adc_sample_rate / adc_block_size;
const uint16_t frame_switch_tres = (update_freq);
const float tframe = (float)adc_block_size / adc_sample_rate;
const float tau = 30e-3;  // 30ms
const uint16_t k = FIX_ONE * expf(-tframe / tau);
const uint16_t one_minus_k = FIX_ONE - k;

static uint16_t mmse_bins[NUM_BINS][MAG_SIZE];
static uint16_t lpf_mags[MAG_SIZE];
static uint16_t bin_idx;
static uint16_t count;
static nc_mode_e mode = nc_mode_soft;

static inline uint16_t mag_min(uint16_t m1, uint16_t m2) {
  return m1 < m2 ? m1 : m2;
}

void noise_canceler_init(void) {
  mode = nc_mode_soft;
  for (uint16_t i = 0; i < NUM_BINS; i++) {
    for (uint16_t j = 0; j < MAG_SIZE; j++) {
      mmse_bins[i][j] = FIX_ONE;
    }
  }
}

void noise_canceler_set_mode(nc_mode_e m) { mode = m; }

void noise_canceler_update(uint16_t mags[]) {
  for (uint16_t i = 0; i < MAG_SIZE; i++) {
    if (mode == nc_mode_soft) {
      lpf_mags[i] =
          ((one_minus_k * (uint32_t)mags[i]) + (k * (uint32_t)lpf_mags[i])) >>
          15;
      mags[i] = lpf_mags[i];
    }
    // Omit zero magnitudes from the analysis completely,
    // as they are not meaningful
    if (mags[i] == 0) {
      continue;
    }

    mmse_bins[bin_idx][i] = mag_min(mmse_bins[bin_idx][i], mags[i]);

    // Minimum Magnitude Spectral Estimate
    uint32_t mmse = mmse_bins[0][i];
    for (uint16_t j = 1; j < NUM_BINS; j++) {
      mmse = mag_min(mmse, mmse_bins[j][i]);
    }

    const uint32_t snr = mmse > 0 ? ((mags[i] << 16) / mmse) : SNR_LIN_HIGH + 1;
    uint32_t adaptive_alpha;
    if (snr < SNR_LIN_LOW) {
      adaptive_alpha = ALPHA_HIGH;
    } else if (snr > SNR_LIN_HIGH) {
      adaptive_alpha = ALPHA_LOW;
    } else {
      uint16_t idx = (snr - SNR_LIN_LOW) / SNR_LUT_SCALE;
      adaptive_alpha = alpha_lut[idx];
    }

    const uint32_t alpha = (mode == nc_mode_soft) ? adaptive_alpha : ALPHA;
    const uint32_t r = (alpha * mmse) / mags[i];
    const uint32_t s = LAMBDA;
    if (r > (FIX_ONE - s)) {
      mags[i] = s;
    } else {
      mags[i] = FIX_ONE - r;
    }
  }

  count++;
  if (count == frame_switch_tres) {
    bin_idx = (bin_idx + 1) % NUM_BINS;
    count = 0;
    for (uint16_t i = 0; i < MAG_SIZE; i++) {
      mmse_bins[bin_idx][i] = FIX_ONE;
    }
  }
}
