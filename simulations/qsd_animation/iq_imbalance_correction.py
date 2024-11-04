from matplotlib import pyplot as plt
import numpy as np

import matplotlib.animation as animation

def sim(f, a1, a2, p):
  t = np.arange(256) * 2 * np.pi / 256.0
  signal = 1.0j*a2*np.sin(10*t) + a1*np.cos(10*t + p*np.pi)
  return signal

signal = sim(11, 1, 1, 0)

my_dpi=96
fig, ax = plt.subplots(5, 1, sharex=True, tight_layout=True)
fig.set_size_inches(960/my_dpi, 1080/my_dpi)

ax[0].set_title('IQ Signal')
ax[0].set_ylim(-2, 2)
l0 = ax[0].plot(signal.real)
l1 = ax[0].plot(signal.imag)

ax[1].set_title('Theta 1')
ax[1].set_ylim(-2, 2)
l2 = ax[1].plot(signal.imag*np.sign(signal.real))
l3 = ax[1].plot(np.ones(len(signal))*np.mean(abs(signal.imag)*np.sign(signal.real)))

ax[2].set_title('Theta 2')
ax[2].set_ylim(-2, 2)
l4 = ax[2].plot(abs(signal.real))
l5 = ax[2].plot(np.ones(len(signal))*np.mean(abs(signal.real)))

ax[3].set_title('Theta 3')
ax[3].set_ylim(-2, 2)
l6 = ax[3].plot(abs(signal.imag))
l7 = ax[3].plot(np.ones(len(signal))*np.mean(abs(signal.imag)))

ax[4].set_title('FFT (Time -> Frequency Domain)')
l8 = ax[4].plot(abs(np.fft.fftshift(np.fft.fft(signal))))


plt.tight_layout(pad=1.08, h_pad=None, w_pad=None, rect=None)


def update(frame):

  if frame < 10:
    signal = sim(11, 1+(frame/25),1, 0)
  elif frame < 20:
    signal = sim(11, 1, 1+((frame-20)/25), 0)
  else:
    signal = sim(11, 1, 1, (frame-40)/20)

  l0[0].set_ydata(signal.real)
  l1[0].set_ydata(signal.imag)

  l2[0].set_ydata(signal.imag*np.sign(signal.real))
  l3[0].set_ydata(np.ones(len(signal))*np.mean(signal.imag*np.sign(signal.real)))

  l4[0].set_ydata(abs(signal.real))
  l5[0].set_ydata(np.ones(len(signal))*np.mean(abs(signal.real)))

  l6[0].set_ydata(abs(signal.imag))
  l7[0].set_ydata(np.ones(len(signal))*np.mean(abs(signal.imag)))

  l8[0].set_ydata(abs(np.fft.fftshift(np.fft.fft(signal))))

ani = animation.FuncAnimation(fig=fig, func=update, frames=80, interval=10)
ani.save('iq_animation.gif', writer='imagemagick', fps=10, dpi=my_dpi)
plt.show()
