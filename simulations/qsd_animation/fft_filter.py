from matplotlib import pyplot as plt
import numpy as np

import matplotlib.animation as animation

def sim(f):
  t = np.arange(128) * 2 * np.pi / 128.0
  signal = 0.4*np.exp(1.0j*40*t) + 0.5*np.exp(-1.0j*10*t) + 0.5*np.exp(-1.0j*40*t) + 0.5*np.exp(1.0j*12*t) + np.random.normal(0.0, 0.1, 128) + 1.0j*np.random.normal(0.0, 0.1, 128)
  segments = signal.reshape(8, 16)
  return signal, segments

signal, segments = sim(11)

my_dpi=96
fig, ax = plt.subplots(7, 1, sharex=True, tight_layout=True)
fig.set_size_inches(960/my_dpi, 1080/my_dpi)

ax[0].set_title('Signal + Noise and Interfering Signals')
ax[0].set_ylim(-2, 2)
l0 = ax[0].plot(np.arange(0, 128), signal.real)
l1 = ax[0].plot(signal.imag)

ax[1].set_title('Window Segments')
ax[1].set_ylim(-2, 2)
segment = np.concatenate((segments[0], segments[1]))
segment *= np.hamming(32)
l2 = ax[1].plot(segment.real)
l3 = ax[1].plot(segment.imag)

ax[2].set_title('FFT (Time -> Frequency Domain)')
ax[2].set_ylim(-8, 8)
segment_freq = np.fft.fftshift(np.fft.fft(segment))
l4 = ax[2].plot(segment_freq.real)
l5 = ax[2].plot(segment_freq.imag)

ax[3].set_title('Frequency Response')
ax[3].set_ylim(-0.2, 1.2)
filter_response = np.concatenate((np.zeros(16), np.ones(8), np.zeros(8)))
l12 = ax[3].plot(filter_response)

ax[4].set_title('Filter (Multiply by Frequency Response)')
ax[4].set_ylim(-8, 8)
segment_freq *= filter_response
l6 = ax[4].plot(segment_freq.real)
l7 = ax[4].plot(segment_freq.imag)

ax[5].set_title('IFFT (Freqeuncy -> Time Domain)')
ax[5].set_ylim(-1, 1)
segment = np.fft.ifft(np.fft.fftshift(segment_freq))
l8 = ax[5].plot(segment.real)
l9 = ax[5].plot(segment.imag)

ax[6].set_title('Filtered Signal')
ax[6].set_ylim(-1, 1)
l10 = ax[6].plot(segment.real)
l11 = ax[6].plot(segment.imag)

plt.tight_layout(pad=1.08, h_pad=None, w_pad=None, rect=None)


filtered_segment = np.zeros(256, dtype="complex128")
def update(frame):
  global filtered_segment
  if frame == 0:
    filtered_segment = np.zeros(256, dtype="complex128")

  segment = np.concatenate((segments[frame], segments[frame+1]))
  segment *= np.hamming(32)
  l2[0].set_ydata(segment.real)
  l3[0].set_ydata(segment.imag)
  l2[0].set_xdata(np.arange(16*frame, 16*frame+32))
  l3[0].set_xdata(np.arange(16*frame, 16*frame+32))

  segment_freq = np.fft.fftshift(np.fft.fft(segment))
  l4[0].set_ydata(segment_freq.real)
  l5[0].set_ydata(segment_freq.imag)
  l4[0].set_xdata(np.arange(16*frame, 16*frame+32))
  l5[0].set_xdata(np.arange(16*frame, 16*frame+32))

  l12[0].set_xdata(np.arange(16*frame, 16*frame+32))

  l6[0].set_ydata((segment_freq*filter_response).real)
  l7[0].set_ydata((segment_freq*filter_response).imag)
  l6[0].set_xdata(np.arange(16*frame, 16*frame+32))
  l7[0].set_xdata(np.arange(16*frame, 16*frame+32))

  segment = np.fft.ifft(np.fft.fftshift(segment_freq*filter_response))
  l8[0].set_ydata(segment.real)
  l9[0].set_ydata(segment.imag)
  l8[0].set_xdata(np.arange(16*frame, 16*frame+32))
  l9[0].set_xdata(np.arange(16*frame, 16*frame+32))

  filtered_segment[16*frame:16*frame+32] = filtered_segment[16*frame:16*frame+32] + segment
  x = np.arange(0, len(filtered_segment))
  l10[0].set_ydata(filtered_segment.real)
  l11[0].set_ydata(filtered_segment.imag)
  l10[0].set_xdata(x)
  l11[0].set_xdata(x)

ani = animation.FuncAnimation(fig=fig, func=update, frames=7, interval=500)
ani.save('fft_animation.gif', writer='imagemagick', fps=2, dpi=my_dpi)
plt.show()
