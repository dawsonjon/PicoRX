from math import log, pi, ceil, log2
from matplotlib import pyplot as plt
import numpy as np
from scipy import signal
from subprocess import run

signal =  20*np.exp(1.0j*10e3*np.arange(1000)*2*np.pi/500e3)
signal += 10*np.exp(1.0j*20e3*np.arange(1000)*2*np.pi/500e3)
signal += 5*np.exp(1.0j*30e3*np.arange(1000)*2*np.pi/500e3)
signal += 1*np.exp(1.0j*40e3*np.arange(1000)*2*np.pi/500e3)
signal += 1*np.exp(1.0j*-10e3*np.arange(1000)*2*np.pi/500e3)
signal += 0.5*np.exp(1.0j*-20e3*np.arange(1000)*2*np.pi/500e3)
signal += 0.25*np.exp(1.0j*-30e3*np.arange(1000)*2*np.pi/500e3)
signal += 0.125*np.exp(1.0j*-40e3*np.arange(1000)*2*np.pi/500e3)
data_raw = [i.imag if (x & 1) else i.real for x, i in enumerate(signal)]
data_raw = np.array(data_raw)*20+2048
data_raw = " ".join([hex(int(i))[2:] for i in data_raw])
print(data_raw)

run(["g++", "rx_dsp.cpp", "test_dsp.cpp", "half_band_filter.cpp", "half_band_filter2.cpp", "-o", "test_dsp"])
output = run("./test_dsp", input=bytes(data_raw, "utf8"), capture_output=True)
error = output.stderr.decode("utf8").strip()
for line in error.splitlines():
  print(line)
output = output.stdout.decode("utf8").strip()

samples = [complex(i) for i in output.split(",")[:-1]]
samples = np.array(samples)
spectrum = abs(np.fft.fftshift(np.fft.fft(np.concatenate([samples, np.zeros(0)]))))
spectrum = [0 if i == 0 else 20*np.log10(i) for i in spectrum]
#
samples_raw = [int(i, 16) for i in data_raw.split()]
samples_raw = [i-np.mean(samples_raw) for i in samples_raw]
samples_raw = [1.0j*i if (x & 1) else i for x, i in enumerate(samples_raw)]
samples_raw = np.array(samples_raw)
spectrum_raw = 20*np.log10(abs(np.fft.fftshift(np.fft.fft(samples_raw))))
#
#
decimation_ratio = len(spectrum)/len(spectrum_raw)
plt.psd(samples_raw, Fs=500e3)
plt.psd(samples, Fs=500e3*decimation_ratio)
plt.show()
plt.plot(samples.real)
plt.plot(samples.imag)
plt.show()
