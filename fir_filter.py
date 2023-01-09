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
    decimation = 20
    taps1=27
    taps2=63

    plt.figure()
    plt.grid(True)
    plt.title("Decimation CIC=%i x filter1=%i"%(decimation, 2))
    plt.xlabel("Frequency (kHz)")
    plt.ylabel("Gain (dB)")
    plt.ylim(-150, 10)
    plot_cic(decimation, 4, fs_kHz) #cic decimation filter
    plot_kernel(0.5, taps1, decimation, fs_kHz/(decimation), True, "Half Band FIR Filter 1") #first decimation filter
    dB3attenuation = plot_kernel(0.5, taps2, decimation, fs_kHz/(2*decimation), False, "Half Band FIR Filter 2")#second filter
    dB3_point = fs_kHz/(8*decimation)
    plt.plot([dB3_point, dB3_point], [-150, 10], linestyle="dashed", color="orange")
    plt.plot([-dB3_point, -dB3_point], [-150, 10], linestyle="dashed", color="orange")
    plt.plot([-dB3_point, dB3_point], [dB3attenuation, dB3attenuation], "x-")
    plt.text(dB3_point, dB3attenuation, "%0.2f (+/-%0.2f) kHz"%(dB3_point*2, dB3_point))
    plt.legend()
    plt.show()

    print("FIR filter 1")
    kernel = make_kernel(0.5, taps1, decimation)
    kernel = [int(i) for i in kernel]
    print(kernel)

    print("FIR Filter 2")
    kernel = make_kernel(0.5, taps2, decimation)
    kernel = [int(i) for i in kernel]
    print(kernel)
