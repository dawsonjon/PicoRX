from math import log, pi, ceil, log2
from matplotlib import pyplot as plt
import numpy as np
from scipy import signal
import sys

def make_kernel(freq, taps, kernel_bits):
    kernel = signal.firwin(taps, freq, window="blackman")
    kernel = np.round(kernel * (2.0**(kernel_bits-1.0)))
    return kernel

def frequency_response(kernel, kernel_bits):
    response = np.concatenate([kernel, np.zeros(1024)])
    response /= (2.0**kernel_bits - 1.0) 
    #response = response[0::2]
    response = 20*np.log10(abs(np.fft.fftshift(np.fft.fft(response))))
    return response

def plot_kernel(freq, taps, kernel_bits, fs):
    response_0 = frequency_response(make_kernel(freq, taps, kernel_bits), kernel_bits)  #Narrow

    plt.figure()

    plt.grid(True)
    plt.title("Narrow")
    plt.xlabel("Frequency (kHz)")
    plt.ylabel("Gain (dB)")
    plt.plot(
            np.linspace(-fs/2, fs/2, len(response_0)), 
            response_0
    )
    plt.plot(
            np.linspace(-fs/2, fs/2, len(response_0)), 
            np.fft.fftshift(response_0)
    )
    plt.show()


if __name__ == "__main__":
    decimation = 8
    plot_kernel(0.5, 27, 16, 500/decimation)

    taps = 27
    kernel = make_kernel(0.5, taps, 16)
    buffer_mask = (2**ceil(log2(taps)))-1

    kernel = [int(i) for i in kernel]
    print(kernel)
