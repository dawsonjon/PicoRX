from matplotlib import pyplot as plt
import numpy as np

import matplotlib.animation as animation

def rc_filter(x):
  output = []
  o = 0
  for i in x:
    o = o * 0.8 + i * 0.2
    output.append(o)
  return np.array(output)

def sim(f):
  t = np.arange(256) * 2 * np.pi / 256.0
  signal = np.sin(f*t)
  lo = (np.arange(256)//4)%4

  s0 = [s if i == 0 else 0 for i, s in zip(lo, signal)]
  s1 = [s if i == 2 else 0 for i, s in zip(lo, signal)]
  s2 = [s if i == 1 else 0 for i, s in zip(lo, signal)]
  s3 = [s if i == 3 else 0 for i, s in zip(lo, signal)]
  return signal, lo, s0, s1, s2, s3

signal, lo, s0, s1, s2, s3 = sim(14)

my_dpi=96
fig, ax = plt.subplots(7, 1, sharex=True, tight_layout=True)
fig.set_size_inches(960/my_dpi, 1080/my_dpi)

ax[0].set_title('RF Input')
signal_line = ax[0].plot(signal)
ax[0].fill_between(np.arange(256), -1, 1, where=lo == 0, facecolor='red', alpha=.3)
ax[0].fill_between(np.arange(256), -1, 1, where=lo == 2, facecolor='orange', alpha=.3)
ax[0].fill_between(np.arange(256), -1, 1, where=lo == 1, facecolor='green', alpha=.3)
ax[0].fill_between(np.arange(256), -1, 1, where=lo == 3, facecolor='blue', alpha=.3)

ax[1].set_title('First Quarter Sample')
s0_line = ax[1].plot(s0, 'red')
s0_line_f = ax[1].plot(rc_filter(s0), 'purple')
ax[1].fill_between(np.arange(256), -1, 1, where=lo == 0, facecolor='red', alpha=.3)

ax[2].set_title('Second Quarter Sample')
s2_line = ax[2].plot(s2, 'green')
s2_line_f = ax[2].plot(rc_filter(s2), 'purple')
ax[2].fill_between(np.arange(256), -1, 1, where=lo == 1, facecolor='green', alpha=.3)

ax[3].set_title('Third Quarter Sample')
s1_line = ax[3].plot(s1, 'orange')
s1_line_f = ax[3].plot(rc_filter(s1), 'purple')
ax[3].fill_between(np.arange(256), -1, 1, where=lo == 2, facecolor='orange', alpha=.3)

ax[4].set_title('Fourth Quarter Sample')
s3_line = ax[4].plot(s3, 'blue')
s3_line_f = ax[4].plot(rc_filter(s3), 'purple')
ax[4].fill_between(np.arange(256), -1, 1, where=lo == 3, facecolor='blue', alpha=.3)

ax[5].set_title('I Output (to ADC)')
i_line = ax[5].plot(rc_filter(s0) - rc_filter(s1), 'red')
i_line_f = ax[5].plot(rc_filter(rc_filter(s0) - rc_filter(s1)), 'purple')

ax[6].set_title('Q Output (to ADC)')
q_line = ax[6].plot(rc_filter(s2) - rc_filter(s3), 'green')
q_line_f = ax[6].plot(rc_filter(rc_filter(s2) - rc_filter(s3)), 'purple')


def update(frame):
  signal, lo, s0, s1, s2, s3 = sim(13+frame/10)
  signal_line[0].set_ydata(signal)
  s0_line[0].set_ydata(s0)
  s0_line_f[0].set_ydata(rc_filter(s0))
  s1_line[0].set_ydata(s1)
  s1_line_f[0].set_ydata(rc_filter(s1))
  s2_line[0].set_ydata(s2)
  s2_line_f[0].set_ydata(rc_filter(s2))
  s3_line[0].set_ydata(s3)
  s3_line_f[0].set_ydata(rc_filter(s3))
  i_line[0].set_ydata(rc_filter(s0) - rc_filter(s1))
  i_line_f[0].set_ydata(rc_filter(rc_filter(s0) - rc_filter(s1)))
  q_line[0].set_ydata(rc_filter(s2) - rc_filter(s3))
  q_line_f[0].set_ydata(rc_filter(rc_filter(s2) - rc_filter(s3)))
  return signal_line

ani = animation.FuncAnimation(fig=fig, func=update, frames=60, interval=30)
ani.save('animation.gif', writer='imagemagick', fps=30, dpi=my_dpi)
plt.show()
