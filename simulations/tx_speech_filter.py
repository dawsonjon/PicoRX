import math
import numpy as np
import scipy.signal as sig

# import scipy.io.wavfile as wv
import matplotlib.pyplot as plt

from cffi import FFI

BIQUAD_SRC = """
{
  static int16_t x1 = 0;
  static int16_t y1 = 0;
  static int16_t x2 = 0;
  static int16_t y2 = 0;
  static int16_t err = 0;

  x >>= 1;
  int32_t y = ((int32_t)x * taps[0][0]) +
              ((int32_t)x1 * taps[0][1]) +
              ((int32_t)x2 * taps[0][2]) + err;
  y -= ((int32_t)y1 * taps[1][1]) + ((int32_t)y2 * taps[1][2]);

  if (y > 0xFFFFFFFL)
  {
    y = 0xFFFFFFFL;
  }
  if (y < -0x10000000L)
  {
    y = -0x10000000L;
  }

  err = y & ((1 << 14) - 1);
  y = (int16_t)(y >> 14);

  x2 = x1;
  x1 = x;
  y2 = y1;
  y1 = y;
  return y << 1;
}
"""

SR = 10000
FIX_ONE = (1 << 14) - 1


def get_C_filter(b, a, name, ffi):

    b = ", ".join(map(str, b))
    a = ", ".join(map(str, a))

    SRCS = "\n".join(
        [
            "static const int32_t taps[2][3] = {",
            f"{{{b}}},",
            f"{{{a}}},",
            "};",
            f"int16_t {name}(int16_t x)",
        ]
        + [BIQUAD_SRC]
    )

    ffi.set_source(
        "_filter",
        SRCS,
        source_extension=".c",
    )

    ffi.cdef(f"int16_t {name}(int16_t x);")
    ffi.compile()
    from _filter import lib

    return lib


def low_shelf(f0, g):
    S = 1
    A = math.pow(10, g / 40)
    w0 = 2 * math.pi * f0 / SR
    alpha = math.sin(w0) / 2 * math.sqrt((A + 1 / A) * (1 / S - 1) + 2)

    b0 = A * ((A + 1) - (A - 1) * math.cos(w0) + 2 * math.sqrt(A) * alpha)
    b1 = 2 * A * ((A - 1) - (A + 1) * math.cos(w0))
    b2 = A * ((A + 1) - (A - 1) * math.cos(w0) - 2 * math.sqrt(A) * alpha)
    a0 = (A + 1) + (A - 1) * math.cos(w0) + 2 * math.sqrt(A) * alpha
    a1 = -2 * ((A - 1) + (A + 1) * math.cos(w0))
    a2 = (A + 1) + (A - 1) * math.cos(w0) - 2 * math.sqrt(A) * alpha

    return [b0 / a0, b1 / a0, b2 / a0], [1.0, a1 / a0, a2 / a0]


def high_shelf(f0, g):
    S = 1
    A = math.pow(10, g / 40)
    w0 = 2 * math.pi * f0 / SR
    alpha = math.sin(w0) / 2 * math.sqrt((A + 1 / A) * (1 / S - 1) + 2)

    b0 = A * ((A + 1) + (A - 1) * math.cos(w0) + 2 * math.sqrt(A) * alpha)
    b1 = -2 * A * ((A - 1) + (A + 1) * math.cos(w0))
    b2 = A * ((A + 1) + (A - 1) * math.cos(w0) - 2 * math.sqrt(A) * alpha)
    a0 = (A + 1) - (A - 1) * math.cos(w0) + 2 * math.sqrt(A) * alpha
    a1 = 2 * ((A - 1) - (A + 1) * math.cos(w0))
    a2 = (A + 1) - (A - 1) * math.cos(w0) - 2 * math.sqrt(A) * alpha

    return [b0 / a0, b1 / a0, b2 / a0], [1.0, a1 / a0, a2 / a0]

def high_pass(f0, g):
    S = 1
    A = math.pow(10, g / 40)
    w0 = 2 * math.pi * f0 / SR
    alpha = math.sin(w0) / 2 * math.sqrt((A + 1 / A) * (1 / S - 1) + 2)

    b0 = (1 + math.cos(w0))/2
    b1 = -(1 + math.cos(w0))
    b2 = (1 + math.cos(w0))/2
    a0 = 1 + alpha
    a1 = -2 * math.cos(w0)
    a2 = 1 - alpha

    return [b0 / a0, b1 / a0, b2 / a0], [1.0, a1 / a0, a2 / a0]


def add_plot(b, a, ax1, ax2):
    w, h = sig.freqz(b, a, fs=SR)
    ax1.plot(w, 20 * np.log10(abs(h)), "C0", label=f"{g}dB")
    phase = np.unwrap(np.angle(h))
    ax2.plot(w, phase, "C1")


def print_ba(b, a):

    def format(x):
        return str(int(x))

    b = ", ".join(list(map(format, np.round(np.array(b) * FIX_ONE))))
    a = ", ".join(list(map(format, np.round(np.array(a) * FIX_ONE))))
    print(f"   {{ {{{b}}}, {{{a}}} }},")

_, ax1 = plt.subplots(tight_layout=True)
ax1.set_title("Bass filters")
ax1.grid(True)
ax1.set_xlabel("Frequency in Hz", color="C2")
ax1.set_ylabel("Amplitude in dB", color="C0")
ax2 = ax1.twinx()
ax2.set_ylabel("Phase [rad]", color="C1")

print("// bass cut")
for g in range(-10, 11, 2):
    b, a = low_shelf(1000, g)
    add_plot(b, a, ax1, ax2)
    print_ba(b, a)
plt.show()

_, ax1 = plt.subplots(tight_layout=True)
ax1.set_title("Treble Boost")
ax1.grid(True)
ax1.set_xlabel("Frequency in Hz", color="C2")
ax1.set_ylabel("Amplitude in dB", color="C0")
ax2 = ax1.twinx()
ax2.set_ylabel("Phase [rad]", color="C1")

print("// treble boost")
for g in range(-10, 11, 2):
    b, a = high_shelf(1200, g)
    add_plot(b, a, ax1, ax2)
    print_ba(b, a)
plt.show()
