from math import log, pi, ceil, log2
from matplotlib import pyplot as plt
import numpy as np
from scipy import signal
import sys

def make_kernel(freq, taps, kernel_bits):
    kernel = signal.firwin(taps, freq, window="blackman")
    kernel = kernel / np.sum(np.abs(kernel))
    kernel = np.round(kernel * (2.0**(kernel_bits-1.0)))
    print(sum(np.abs(kernel)))
    print(2.0**kernel_bits-1.0)
    print(sum(np.abs(kernel))/(2.0**(kernel_bits-1.0)))
    return kernel

def frequency_response(kernel, kernel_bits):
    response = np.concatenate([kernel, np.zeros(1024)])
    response /= (2.0**(kernel_bits - 1.0))
    response = 20*np.log10(abs(np.fft.fftshift(np.fft.fft(response))))
    return response

def plot_kernel(freq, taps, kernel_bits, fs, label=""):
    response = frequency_response(make_kernel(freq, taps, kernel_bits), kernel_bits)
    plt.plot(
          np.linspace(-fs/2, fs/2, len(response)),
          response,
          "g-",
          label = label
    )

    return np.interp(fs/4, np.linspace(-fs/2, fs/2, len(response)), response)


if __name__ == "__main__":
    fs_kHz = 500
    bits = 16
    taps1=27
    taps2=63

    plt.figure()
    plt.grid(True)
    plt.xlabel("Frequency (kHz)")
    plt.ylabel("Gain (dB)")
    plt.ylim(-150, 10)
    plot_kernel(0.5, taps1, bits, fs_kHz, "Half Band FIR Filter 1")
    dB3attenuation = plot_kernel(0.5, taps2, bits, fs_kHz, "Half Band FIR Filter 2")#second filter
    plt.legend()
    plt.show()

    print("FIR filter 1")
    kernel = make_kernel(0.5, taps1, bits)
    kernel = [int(i) for i in kernel]
    print(kernel)

    print("FIR Filter 2")
    kernel = make_kernel(0.5, taps2, bits)
    kernel = [int(i) for i in kernel]
    print(kernel)
