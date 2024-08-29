import numpy as np
from matplotlib import pyplot as plt

Fs = 500e3/32
f = 100
samples = 2048

xs = []
phases = []
fadjustments = []

t = 0
gain = 100
f_adjust = 0
for i in range(100):

  t_block = np.arange(t, t+samples)
  x = np.exp((f+f_adjust)/Fs*2.0j*np.pi*t_block/samples)
  phase = np.sum(x.imag)/np.sum(x.real)
  t += samples
  f_adjust = -phase * gain

  print(phase, f_adjust)

  xs += list(x)
  phases.append(phase)
  fadjustments.append(f_adjust)

plt.plot(np.array(xs).real)
plt.plot(np.array(xs).imag)
plt.show()
plt.plot(phases)
plt.plot(fadjustments)
plt.show()
