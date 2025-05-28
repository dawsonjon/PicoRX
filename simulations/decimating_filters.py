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

def upsample(x, n):
    return np.concatenate([[y] + [0] * (n - 1) for y in x])

def mult_poly(p1, p2):
    return np.polymul(p1, p2)

def get_cic_response(length, order):

    #CIC filter is equivilent to moving average filter
    #make an equivilent filter kernel by convolving a rectangular kernel
    h = np.ones(length)/length
    response = h
    for _ in range(order-1):
      response = np.convolve(response, h)
    return response, [1.0]

def get_iir_response(target_decim):

    # IIR-based, 2-branch, polyphase (1:2) decimator
    # how it was designed is out of scope for this script
    b = [
        1.74176794e-01,
        4.16748110e-01,
        6.45175700e-01,
        6.45175700e-01,
        4.16748110e-01,
        1.74176794e-01,
        0.00000000e00,
    ]

    a = [
        2.90351399e-01,
        0.00000000e00,
        1.18184981e00,
        0.00000000e00,
        1.00000000e00,
    ]
    b, a = upsample(b, target_decim // 2), upsample(a, target_decim // 2)

    # additional output IIR filter
    b_out = [6.38945525e-01, 1.27789105e+00, 6.38945525e-01]
    a_out = [1.00000000e+00, 1.14298050e+00, 4.12801598e-01]
    b_out = upsample(b_out, target_decim)
    a_out = upsample(a_out, target_decim)

    b = mult_poly(b, b_out)
    a = mult_poly(a, a_out)

    return b, a

def plot_magnitude(ax, response, fs, label, color='blue'):
    spec_len = 2048
    w, h = signal.freqz(response[0], response[1], worN=spec_len, fs=fs)
    ax.plot(w, 20 * np.log10(np.abs(h)), color=color, label=label)

def plot_wrapped_magnitude(ax, wrap, response, fs, label, color='blue'):

    spec_len = 2048
    _, h1 = signal.freqz(response[0], response[1], worN=spec_len, fs=fs)

    #Fold aliased parts of the signal around Fs/2 
    mag = 20 * np.log10(np.abs(h1))
    frag_len = spec_len // wrap
    fragments = [] 
    for i in range(wrap):
      fragment = mag[i * frag_len : (i + 1) * frag_len]
      if i & 1:
        fragment = np.flip(fragment)
      fragments.append(fragment)

    #Plot each folded alias
    fragment = fragments[0]
    for idx, fragment in enumerate(fragments):
      if idx == 0:
        ax.plot(
          np.linspace(0, fs/(2*wrap), len(fragment)), 
          fragment,
          color,
          label = label
        )
      else:
        ax.plot(
          np.linspace(0, fs/(2*wrap), len(fragment)), 
          fragment,
          color,
        )
      ax.plot(
        np.linspace(0, -fs/(2*wrap), len(fragment)), 
        fragment,
        color
      )

def plot_correction(length, order, fs):

    #CIC filter is equivilent to moving average filter
    #make an equivilent filter kernel by convolving a rectangular kernel
    h = np.ones(length)/length
    response = h
    for i in range(order-1):
      response = np.convolve(response, h)

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
    fs_kHz = 480
    decimation = 16
    taps1=27
    taps2=63

    fig, axs = plt.subplots(2, 1)

    axs[0].set_title("Decimation CIC")
    plt.xlabel("Frequency (kHz)")
    resp = get_cic_response(decimation, 4)
    plot_wrapped_magnitude(axs[0], decimation, resp, fs_kHz, f"Order {4} CIC {16}:1 Decimator")
    plot_magnitude(axs[1], resp, fs_kHz, f"Order {4} CIC {16}:1 Decimator")
    resp = get_cic_response(8, 4)
    plot_wrapped_magnitude(
        axs[0], decimation, resp, fs_kHz, f"Order {4} CIC {8}:1 Decimator", "red"
    )
    plot_magnitude(axs[1], resp, fs_kHz, f"Order {4} CIC {8}:1 Decimator", "red")
    iir_resp = get_iir_response(16)
    resp = mult_poly(resp[0], iir_resp[0]), mult_poly(resp[1], iir_resp[1])

    plot_wrapped_magnitude(axs[0], decimation, resp, fs_kHz, f"CIC {8}:1 + IIR Filter", "green")
    plot_magnitude(axs[1], resp, fs_kHz, f"CIC {8}:1 + IIR Filter", "green")
    for ax in axs:
      ax.grid(True)
      ax.set_ylabel("Gain (dB)")
      ax.set_ylim(-150, 10)
    
    plt.legend()
    plt.show()

    plt.grid(True)
    plt.title("CIC correction CIC=%i"%decimation)
    plt.xlabel("Frequency (kHz)")
    plt.ylabel("Gain")
    plot_correction(decimation, 4, fs_kHz) #cic decimation filter
    plt.legend()
    plt.show()
