import math
from cffi import FFI
import numpy as np
import scipy.signal as sig
import matplotlib.pyplot as plt


SRC = """

int16_t rectangular_2_phase(int16_t i, int16_t q)
{

   //handle condition where phase is unknown
   if(i==0 && q==0) return 0;

   const int16_t absi=i>0?i:-i;
   int16_t angle=0;
   if (q>=0)
   {
      //scale r so that it lies in range -8192 to 8192
      const int16_t r = ((int32_t)(q - absi) << 13) / (q + absi);
      angle = 8192 - r;
   }
   else
   {
      //scale r so that it lies in range -8192 to 8192
      const int16_t r = ((int32_t)(q + absi) << 13) / (absi - q);
      angle = (3 * 8192) - r;
   }

   //angle lies in range -32768 to 32767
   if (i < 0) return(-angle);     // negate if in quad III or IV
   else return(angle);
}

void rectangular_2_polar_1(int16_t i, int16_t q, uint16_t *mag, int16_t *phase)
{
  //Measure magnitude
  const int16_t absi = i>0?i:-i;
  const int16_t absq = q>0?q:-q;
  *mag = absi > absq ? absi + absq / 4 : absq + absi / 4;
  *phase = rectangular_2_phase(q, i);
}

void rectangular_2_polar_f(int16_t i, int16_t q, uint16_t *mag, int16_t *phase)
{
  float i_f = (float)i / 32768;
  float q_f = (float)q / 32768;
  *mag = sqrtf(i_f * i_f + q_f * q_f) * 32768;
  *phase = (atan2f(q, i) / (M_PI)) * 32768;
}

void rectangular_2_polar_2(int16_t i, int16_t q, uint16_t *mag, int16_t *phase) {
  int16_t temp_i;
  int16_t angle = 0;

  if (i < 0) {
    // rotate by an initial +/- 90 degrees
    temp_i = i;
    if (q > 0.0f) {
      i = q; // subtract 90 degrees
      q = -temp_i;
      angle = 16384;
    } else {
      i = -q; // add 90 degrees
      q = temp_i;
      angle = -16384;
    }
  }

  for (uint16_t k = 0; k < CORDIC_ITERS; k++) {
    temp_i = i;
    if (q > 0) {
      /* Rotate clockwise */
      i += (q >> k);
      q -= (temp_i >> k);
      angle += CORDIC_ATAN_LUT[k];
    } else {
      /* Rotate counterclockwise */
      i -= (q >> k);
      q += (temp_i >> k);
      angle -= CORDIC_ATAN_LUT[k];
    }
  }

  *mag = ((uint32_t)i * CORDIC_GAIN) >> 16;
  *phase = angle;
}
"""

iters = 6
gain = np.prod([1 / (math.sqrt(1 + math.pow(2, -2 * i))) for i in range(iters)])
print(f"GAIN: ({gain})")

atan_lut = [str(round(float(32768 * np.atan(2**-i) / np.pi))) for i in range(iters)]
atan_lut = ", ".join(atan_lut)

gain = round(gain * (1 << 16))

SRC = "\n".join(
    [
        f"#define CORDIC_ITERS ({iters})",
        f"static const uint32_t CORDIC_GAIN = {gain};",
        f"static const int16_t CORDIC_ATAN_LUT[CORDIC_ITERS] = {{{atan_lut}}};",
        SRC,
    ]
)

ffi = FFI()

ffi.set_source(
    "_mag",
    SRC,
    source_extension=".cpp",
)

# print(SRC)

ffi.cdef("void rectangular_2_polar_f(int16_t i, int16_t q, uint16_t *mag, int16_t *phase);")
ffi.cdef("void rectangular_2_polar_1(int16_t i, int16_t q, uint16_t *mag, int16_t *phase);")
ffi.cdef("void rectangular_2_polar_2(int16_t i, int16_t q, uint16_t *mag, int16_t *phase);")
ffi.compile()

from _mag import lib

time = np.arange(0, 0.05, 1 / 15000)
s = np.sin(2 * np.pi * 100 * time) * np.sin(2 * np.pi * 10 * time) * 32768 * 0.1
a = sig.hilbert(s)

mag_f = []
mag_1 = []
mag_2 = []
phi_f = []
phi_1 = []
phi_2 = []

m = ffi.new("uint16_t*")
p = ffi.new("int16_t *")

for iq in a:
    i = int(np.real(iq))
    q = int(np.imag(iq))

    lib.rectangular_2_polar_f(i, q, m, p)
    mag_f.append(m[0])
    phi_f.append(p[0])
    lib.rectangular_2_polar_1(i, q, m, p)
    mag_1.append(m[0])
    phi_1.append(p[0])
    lib.rectangular_2_polar_2(i, q, m, p)
    mag_2.append(m[0])
    phi_2.append(p[0])

plt.title("Magnitude")
plt.plot(time, mag_f, label="ground truth")
plt.plot(time, mag_1, label="basic estimator")
plt.plot(time, mag_2, label="cordic estimator")
plt.plot(time, s, label="test signal")
plt.grid(True)
plt.legend()
plt.show()

plt.title("Phase")
plt.plot(time, phi_f, label="ground truth")
plt.plot(time, phi_1, label="basic estimator")
plt.plot(time, phi_2, label="cordic estimator")
plt.plot(time, s, label="test signal")
plt.grid(True)
plt.legend()
plt.show()
