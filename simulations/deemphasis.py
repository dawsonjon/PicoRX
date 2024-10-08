import math

import numpy as np
from scipy import signal
import matplotlib.pyplot as plt


FS = 480000 // 32


def to_fixed(arr):
    return list(map(lambda x: round(x * 32768), arr))


def standard_deemph(tau=50e-6, fs=FS):
    w_c = 1.0 / tau

    w_ca = 2.0 * fs * math.tan(w_c / (2.0 * fs))

    k = -w_ca / (2.0 * fs)
    z1 = -1.0
    p1 = (1.0 + k) / (1.0 - k)
    b0 = -k / (1.0 - k)

    btaps = [b0 * 1.0, b0 * -z1]
    ataps = [1.0, -p1]

    return btaps, ataps


def print_fixed(b, a):
    b, a = to_fixed(b), to_fixed(a)
    print(f"De-emphasis 50us: bs={b}, as={a}")


def plot_mag(title, b, a):
    w, h = signal.freqz(b, a, fs=FS)
    plt.plot(w, 20 * np.log10(abs(h)), "b")
    plt.grid(True)
    plt.xlabel("Hz")
    plt.ylabel("dB")
    plt.title(title)
    plt.show()


b, a = standard_deemph()
print_fixed(b, a)
plot_mag("50us", b, a)

b, a = standard_deemph(75e-6)
print_fixed(b, a)
plot_mag("75us", b, a)
