from math import log, pi, ceil, log2
from matplotlib import pyplot as plt
import numpy as np
from scipy import signal
import sys

def plot_kernel(length, order):
    response = np.ones(length)/length
    for i in range(order-1):
      response = np.convolve(response, response)
    response = np.concatenate([response, np.zeros(500-len(response))])
    response = 20*np.log10(abs(np.fft.fftshift(np.fft.fft(response))))
    response = response[len(response)//2:]

    fragments = [] 
    for i in range(length):
      fragment = response[i*len(response)//length:(i+1)*len(response)//length]
      if i & 1:
        fragment = np.flip(fragment)
      fragments.append(fragment)


    plt.figure()

    plt.grid(True)
    plt.title("Decimation Filter")
    plt.xlabel("Frequency (kHz)")
    plt.ylabel("Gain (dB)")
    plt.ylim(-110, 10)
    for fragment in fragments:
      plt.plot(fragment)
    plt.show()


if __name__ == "__main__":
    plot_kernel(2, 4)

