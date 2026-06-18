from matplotlib import pyplot as plt
import numpy as np
import sys

with open("data_file.txt") as inf:
    data = [[float(i) for i in line.split()] for line in inf]


audio = np.array([i[0] for i in data])
audio_processed = [i[1] for i in data]
envelope = np.array([i[2] for i in data])
audio_filtered = np.array([i[3] for i in data])
raw_i = np.array([i[4] for i in data])
raw_q = np.array([i[5] for i in data])
clipped_i = np.array([i[6] for i in data])
clipped_q = np.array([i[7] for i in data])
complex_envelope = np.array([i[8] for i in data])
i = np.array([i[9] for i in data])
q = np.array([i[10] for i in data])
magnitude = np.array([i[11] for i in data])
phase = np.array([i[12] for i in data])

audio_filtered *= 1.67
raw_i *= 1.67 * 1.67 * 2
raw_q *= 1.67 * 1.67 * 2
raw = raw_i + 1.0j*raw_q
clipped = clipped_i + 1.0j*clipped_q
complex_envelope *= 1.67*1.67*2
output = i + 1.0j*q


#plt.plot(magnitude[:-1], frequency, "x")
#plt.show()

#frequency_conj = output[1:] * np.conj(output[:-1])
#frequency_conj = np.angle(frequency_conj)*-32768/np.pi
#frequency_conj = frequency_conj.astype(np.int16)

plt.title("Reconstructed")
frequency = phase[1:] - phase[:-1]
frequency = frequency.astype(np.int16)

gain = magnitude[1:] / (100+magnitude[1:])
frequency = frequency.astype(float) * gain


phase_reconstructed = np.cumsum(frequency)# + phase[1]
phase_reconstructed = phase_reconstructed.astype(np.int16)
phase_radians = np.pi * phase_reconstructed / 32768
reconstructed = magnitude[1:] * np.exp(-1.0j*phase_radians)/2
#plt.plot(phase[1:], "+-", label="phase")
#plt.plot(phase_reconstructed, "+-", label="phase reconstructed")
#plt.plot(frequency, "+-", label="frequency")
plt.plot(reconstructed.real, "+-", label="recontructed")
plt.plot(output.real[1:], "+-", label="output")
plt.legend()
plt.show()

plt.plot(phase[1:])
plt.plot(phase_reconstructed)
plt.show()
spectrum = np.abs(np.fft.fftshift(np.fft.fft(output)))
plt.plot(20*np.log10(spectrum), label = "output")
spectrum = np.abs(np.fft.fftshift(np.fft.fft(reconstructed)))
plt.plot(20*np.log10(spectrum), label = "reconstructed")
plt.legend()
plt.show()


#plot compressor
audio_peak = np.max(np.abs(audio))
audio /= audio_peak
envelope /= audio_peak
audio_processed /= np.max(np.abs(audio_processed))
plt.title("Audio Compression")
plt.plot(audio_processed, label = "compressed audio")
plt.plot(audio, label = "audio")
plt.plot(envelope, label = "envelope")
plt.legend()
plt.show()
audio_rms = np.sqrt(np.mean(np.abs(audio)**2))
compressed_rms = np.sqrt(np.mean(np.abs(audio_processed)**2))
print("Audio Compression Power Increase (dB) %.1f"%(20*np.log10(compressed_rms/audio_rms)))

plt.title("CESSB")
plt.plot(np.abs(raw), label = "raw")
plt.plot(np.abs(clipped), label = "clipped")
plt.plot(np.abs(output), label = "filtered")
plt.legend()
plt.show()

raw /= np.max(np.abs(raw))
output /= np.max(np.abs(output))
clipped /= np.max(np.abs(clipped))
plt.title("CESSB")
plt.plot(np.abs(raw), label = "raw")
plt.plot(np.abs(output), label = "output")
plt.legend()
plt.show()
raw_rms = np.sqrt(np.mean(np.abs(raw)**2))
output_rms = np.sqrt(np.mean(np.abs(output)**2))
print("CESSB Power Increase (dB) %.1f"%(20*np.log10(output_rms/raw_rms)))

#spectrum = np.abs(np.fft.fftshift(np.fft.fft(audio_processed)))
#plt.plot(spectrum, label = "audio_processed")
spectrum = np.abs(np.fft.fftshift(np.fft.fft(audio_filtered)))
plt.plot(20*np.log10(spectrum), label = "audio_filtered")
#spectrum = np.abs(np.fft.fftshift(np.fft.fft(raw)))
#plt.plot(20*np.log10(spectrum), label = "raw")
spectrum = np.abs(np.fft.fftshift(np.fft.fft(output)))
plt.plot(20*np.log10(spectrum), label = "output")
plt.legend()
plt.show()

