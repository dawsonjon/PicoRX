from math import log, pi, ceil, log2
from matplotlib import pyplot as plt
import numpy as np
from scipy import signal
from subprocess import run

run(["g++", "-DSIMULATION=true", "../utils.cpp", "../fft.cpp", "../fft_filter.cpp", "fft_filter_test.cpp", "-o", "fft_filter_test"])
output = run("./fft_filter_test", capture_output=True)
output = output.stdout.decode("utf8").strip()

i_data = []
q_data = []
for line in output.splitlines():
  _, i, q = [float(i) for i in line.split()]
  i_data.append(i)
  q_data.append(q)

data = np.array(i_data) + 1.0j * np.array(q_data)
data = data[128:]
plt.plot(data.real)
plt.plot(data.imag)
plt.show()
fft = np.fft.fftshift(np.abs(np.fft.fft(data)))
plt.plot(fft)
plt.show()



