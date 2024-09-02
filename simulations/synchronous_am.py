import numpy as np
from matplotlib import pyplot as plt

Fs = 500e3/32
f = 200
samples = 2048

xs = []
phases = []
fadjustments = []

t = 0
gain = 1
fadjust = 0
for i in range(5000):

  t_block = np.arange(t, t+samples)
  x = -np.exp((f+fadjust)/Fs*2.0j*np.pi*t_block/samples)
  t += samples
  #if np.sum(x.real) > 0:
  phase = np.arctan2(np.sum(x.imag), np.sum(x.real))
  

  fadjust += -phase * gain
  #if f_adjust > 200:
  #  f_adjust = 200
  #elif f_adjust < -200:
  #  f_adjust = -200

  xs += list(x)
  phases.extend(np.ones(samples)*phase)
  fadjustments.append(fadjust)

plt.plot(np.array(xs).real)
plt.plot(np.array(xs).imag)
plt.plot(phases)
plt.show()

plt.plot(fadjustments)
plt.show()
