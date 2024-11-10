from matplotlib import pyplot as plt
import numpy as np

import matplotlib.animation as animation

def sim(f1, f2):
  t = np.arange(256) * 2 * np.pi / 256.0
  signal = np.exp(1.0j*t*f1) * (1+np.sin(t*f2))
  signal2 = (1+np.sin(t*f2))
  return signal, signal2

signal, signal2 = sim(1, 20)

my_dpi=96
fig, ax = plt.subplots(2, 2, tight_layout=True)
fig.set_size_inches(960/my_dpi, 1080/my_dpi)

ax[0][0].set_title('IQ Signal (PLL input)')
ax[0][0].set_ylabel('Amplitude')
ax[0][0].set_xlabel('Time (samples)')
ax[0][0].set_ylim(-2, 2)
ax[0][0].set_xlim(0, 256)
l0 = ax[0][0].plot(signal.real[0], label="I")
l1 = ax[0][0].plot(signal.imag[0], label="Q")

ax[0][1].set_title('IQ Plot (PLL input)')
ax[0][1].set_ylim(-2, 2)
ax[0][1].set_xlim(-2, 2)
ax[0][1].set_ylabel('Q')
ax[0][1].set_xlabel('I')
l2 = ax[0][1].plot([0, signal.real[0]], [0, signal.imag[0]], 'bo-')

ax[1][0].set_title('IQ Signal (PLL output)')
ax[1][0].set_ylabel('Amplitude')
ax[1][0].set_xlabel('Time (samples)')
ax[1][0].set_ylim(-2, 2)
ax[1][0].set_xlim(0, 256)
l3 = ax[1][0].plot(signal2.real[0], label="I")
l4 = ax[1][0].plot(signal2.imag[0], label="Q")

ax[1][1].set_title('IQ Plot (PLL output)')
ax[1][1].set_ylim(-2, 2)
ax[1][1].set_xlim(-2, 2)
ax[1][1].set_ylabel('Q')
ax[1][1].set_xlabel('I')
l5 = ax[1][1].plot([0, signal2.real[0]], [0, signal2.imag[0]], 'bo-')

plt.tight_layout(pad=1.08, h_pad=None, w_pad=None, rect=None)


def update(frame):

  l0[0].set_xdata(np.arange(frame))
  l1[0].set_xdata(np.arange(frame))
  l0[0].set_ydata(signal.real[:frame])
  l1[0].set_ydata(signal.imag[:frame])

  l2[0].set_xdata([0, signal.real[frame]])
  l2[0].set_ydata([0, signal.imag[frame]])

  l3[0].set_xdata(np.arange(frame))
  l4[0].set_xdata(np.arange(frame))
  l3[0].set_ydata(signal2.real[:frame])
  l4[0].set_ydata(signal2.imag[:frame])

  l5[0].set_xdata([0, signal2.real[frame]])
  l5[0].set_ydata([0, signal2.imag[frame]])


ani = animation.FuncAnimation(fig=fig, func=update, frames=256, interval=20)
#ani.save('sam.gif', writer='imagemagick', fps=10, dpi=my_dpi)
ani.save('sam.gif', writer='pillow', fps=10, dpi=my_dpi)
plt.show()
