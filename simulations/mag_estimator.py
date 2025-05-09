import math
from cffi import FFI
import numpy as np
import scipy.signal as sig
import matplotlib.pyplot as plt


SRC = """

uint16_t rectangular_2_magnitude_1(int16_t i, int16_t q)
{
  //Measure magnitude
  const int16_t absi = i>0?i:-i;
  const int16_t absq = q>0?q:-q;
  return absi > absq ? absi + absq / 4 : absq + absi / 4;
}

uint16_t rectangular_2_magnitude_f(int16_t i, int16_t q)
{
  float i_f = (float)i / 32767;
  float q_f = (float)q / 32767;
  return sqrtf(i_f * i_f + q_f * q_f) * 32767;
}

uint16_t rectangular_2_magnitude_2(int16_t i, int16_t q) {
  int16_t tempI;

  if (i < 0 && q >= 0) {
    i = std::abs(i);
  } else if (i < 0 && q < 0) {
    i = std::abs(i);
    q = std::abs(q);
  }

  for (uint16_t k = 0; k < ITERS; k++) {
    tempI = i;
    if (q > 0) {
      /* Rotate clockwise */
      i += (q >> k);
      q -= (tempI >> k);
    } else {
      /* Rotate counterclockwise */
      i -= (q >> k);
      q += (tempI >> k);
    }
  }
  uint16_t m = ((uint32_t)i * GAIN) >> 16;
  return m;
}
"""

iters = 4
gain = np.prod([1 / (math.sqrt(1 + math.pow(2, -2 * i))) for i in range(iters)])
gain += 0.0015
print(f"GAIN: ({gain})")

SRC = "\n".join(
    [
        f"#define ITERS ({iters})",
        f"static const uint32_t GAIN = roundf({gain}f * (1 << 16));",
        SRC,
    ]
)

ffi = FFI()

ffi.set_source(
    "_mag",
    SRC,
    source_extension=".cpp",
)

ffi.cdef("uint16_t rectangular_2_magnitude_f(int16_t i, int16_t q);")
ffi.cdef("uint16_t rectangular_2_magnitude_1(int16_t i, int16_t q);")
ffi.cdef("uint16_t rectangular_2_magnitude_2(int16_t i, int16_t q);")
ffi.compile()

from _mag import lib

time = np.arange(0, 0.05, 1 / 15000)
s = np.sin(2 * np.pi * 100 * time) * np.sin(2 * np.pi * 10 * time) * 32767 * 0.1
a = sig.hilbert(s)

mag_f = []
mag_1 = []
mag_2 = []

for iq in a:
    i = int(np.real(iq))
    q = int(np.imag(iq))

    m = lib.rectangular_2_magnitude_f(i, q)
    mag_f.append(m)
    m = lib.rectangular_2_magnitude_1(i, q)
    mag_1.append(m)
    m = lib.rectangular_2_magnitude_2(i, q)
    mag_2.append(m)

plt.plot(time, mag_f, label="ground truth")
plt.plot(time, mag_1, label="basic estimator")
plt.plot(time, mag_2, label="cordic estimator")
plt.plot(time, s, label="test signal")
plt.grid(True)
plt.legend()
plt.show()
