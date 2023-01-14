from math import exp, pi, floor, log
from matplotlib import pyplot as plt

#single pole high pass filter

fs = 12500
factor=7 
x = 1-(1/(1<<factor))
fc = log(x)/(-2*pi)
print("x", x, "fc", fc, "cutoff", fs*fc)

cutoffHz = 100

fc = cutoffHz/fs
x = exp(-2*pi*fc)

a0 = floor(a0*32768)
a1 = floor(a1*32768)
b1 = floor(b1*32768)
print("x", x, "a0", a0, "a1", a1, "b1", b1)


impulse = [32768] + [0]*1023
samples = []
last_i = 0
last_sample = 0
error = 0
for i in impulse:
  sample = ((i * a0) + (last_i * a1) + ((last_sample * b1) + error))
  sample_truncated = sample >> 15
  error = sample - (sample_truncated << 15)
  last_i = i
  last_sample = sample_truncated
  samples.append(sample_truncated)
  print(i, last_i, last_sample, error)
  #print(i*a0)
 
plt.plot(impulse)
plt.plot(samples)
plt.show()
