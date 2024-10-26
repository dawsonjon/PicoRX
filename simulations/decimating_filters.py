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
    response /= (2.0**(kernel_bits - 1.0)) 
    response = 20*np.log10(abs(np.fft.fftshift(np.fft.fft(response))))
    return response

def plot_cic(length, order, fs):

    #CIC filter is equivilent to moving average filter
    #make an equivilent filter kernel by convolving a rectangular kernel
    response = np.ones(length)/length
    for i in range(order-1):
      response = np.convolve(response, response)

    #pad kernel and find magnitude spectrum
    response = np.concatenate([response, np.zeros((500*length)-len(response))])
    response = 20*np.log10(abs(np.fft.fftshift(np.fft.fft(response))))

    #Fold aliased parts of the signal around Fs/2 
    response = response[len(response)//2:]
    fragments = [] 
    for i in range(length):
      fragment = response[i*len(response)//length:(i+1)*len(response)//length]
      if i & 1:
        fragment = np.flip(fragment)
      fragments.append(fragment)

    #Plot each folded alias
    fragment = fragments[0]
    for idx, fragment in enumerate(fragments):
      if idx == 0:
        plt.plot(
          np.linspace(0, fs/(2*length), len(fragment)), 
          fragment,
          "b-",
          label = "Order %u CIC Decmator"%order
        )
      else:
        plt.plot(
          np.linspace(0, fs/(2*length), len(fragment)), 
          fragment,
          "b-",
        )
      plt.plot(
        np.linspace(0, -fs/(2*length), len(fragment)), 
        fragment,
        "b-"
      )

def plot_correction(length, order, fs):

    #CIC filter is equivilent to moving average filter
    #make an equivilent filter kernel by convolving a rectangular kernel
    response = np.ones(length)/length
    for i in range(order-1):
      response = np.convolve(response, response)

    #pad kernel and find magnitude spectrum
    response = np.concatenate([response, np.zeros((256*length)-len(response))])
    response = np.fft.fftshift(1.0/abs(np.fft.fft(response)))

    #Fold aliased parts of the signal around Fs/2 
    response = response[len(response)//2:]
    fragment = response[:(len(response)//length)+1]

    print(",".join([str(int(round(256*i))) for i in fragment]))

    plt.plot(
      np.linspace(0, fs/(2*length), len(fragment)), 
      fragment,
      "b-",
      label = "Order %u CIC Decmator"%order
    )

def plot_kernel(freq, taps, kernel_bits, fs, decimate=False, label=""):
    response = frequency_response(make_kernel(freq, taps, kernel_bits), kernel_bits)

    if decimate:
      length = 2

      response = response[len(response)//2:]
      fragments = [] 

      #Fold aliased parts of the signal around Fs/2 
      for i in range(length):
        fragment = response[i*len(response)//length:(i+1)*len(response)//length]
        if i & 1:
          fragment = np.flip(fragment)
        fragments.append(fragment)

      #Plot each folded alias
      for idx, fragment in enumerate(fragments):
        if idx == 0:
          plt.plot(
            np.linspace(0, fs/(2*length), len(fragment)), 
            fragment,
            "r-",
            label = label
          )
        else:
          plt.plot(
            np.linspace(0, fs/(2*length), len(fragment)), 
            fragment,
            "r-"
          )
        plt.plot(
          np.linspace(0, -fs/(2*length), len(fragment)), 
          fragment,
          "r-"
        )

    else:
      plt.plot(
              np.linspace(-fs/2, fs/2, len(response)), 
              response,
              "g-",
              label = label
      )

      return np.interp(fs/4, np.linspace(-fs/2, fs/2, len(response)), response)


if __name__ == "__main__":
    fs_kHz = 500
    decimation = 16
    taps1=27
    taps2=63

    plt.figure()
    plt.grid(True)
    plt.title("Decimation CIC=%i"%decimation)
    plt.xlabel("Frequency (kHz)")
    plt.ylabel("Gain (dB)")
    plt.ylim(-150, 10)
    plot_cic(decimation, 4, fs_kHz) #cic decimation filter
    plt.legend()
    plt.show()

    plt.figure()
    plt.grid(True)
    plt.title("CIC correction CIC=%i"%decimation)
    plt.xlabel("Frequency (kHz)")
    plt.ylabel("Gain")
    plot_correction(decimation, 4, fs_kHz) #cic decimation filter
    plt.legend()
    plt.show()
