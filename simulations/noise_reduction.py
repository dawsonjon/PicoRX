import numpy as np
from matplotlib import pyplot as plt
import matplotlib.animation as animation

from copy import copy


Fs = 15000


with open("test.wav", "rb") as inf:
  with open("result.wav", "wb") as outf:

    plot_magnitudes = []
    plot_noise = []
    plot_signal_level = []
    plot_gain = []
    plot_output = []
    snr_bin = []


    input_overlap = np.zeros(64)
    output_overlap = np.zeros(64)
    signal_level = np.zeros(128)
    noise_level = np.ones(128)*100000
    for i in range(10000):
      buffer = inf.read(128)
      if len(buffer) < 128:
        break
      samples = np.frombuffer(buffer, "int16") #64 samples from file

      #window and overlaping input segments then convert to frequency domain
      input_frame = np.concatenate([input_overlap, samples]) #128 samples
      input_overlap = samples
      input_frame = np.sqrt(np.hanning(len(input_frame))) * input_frame #analysis window
      input_frame = np.fft.fft(input_frame)

      magnitudes = np.abs(np.fft.fftshift(input_frame))
      signal_level = signal_level.astype("int") + ((magnitudes.astype("int") - signal_level.astype("int")) >> 3)
      noise_level[signal_level <= noise_level] = signal_level[signal_level <= noise_level]
      noise_level[signal_level > noise_level] = np.minimum(noise_level + (noise_level.astype("int") >> 9), signal_level)[signal_level > noise_level]

      #time smoothing
      threshold = 3.0
      gain = np.clip((1-(threshold*noise_level/signal_level)), 0, 1)
      output_frame = input_frame * np.fft.fftshift(gain)

      #plot gains for 1 bin
      plot_magnitudes.append(magnitudes[64:])
      noise = copy(noise_level[64:])
      plot_noise.append(noise)
      plot_signal_level.append(signal_level[64:])
      plot_gain.append(gain[64:])
      plot_output.append(np.fft.fftshift(np.abs(output_frame))[64:])

      #add overlaping output segments then convert to frequency domain
      output_frame = np.fft.ifft(output_frame)
      output_frame = np.sqrt(np.hanning(len(output_frame))) * output_frame #analysis window
      audio = output_frame[:64] + output_overlap
      output_overlap = output_frame[64:]
      audio = audio.real
      audio = audio.astype("int16")
      outf.write(audio.tobytes())

my_dpi=96
fig, ax = plt.subplots(2, 2, tight_layout=True)
fig.set_size_inches(1920/my_dpi, 1080/my_dpi)
num_frames = len(plot_magnitudes)

ax[0][0].set_title('Input Spectrum')
ax[0][0].set_ylabel('Magnitude dB')
ax[0][0].set_xlabel('Frequency kHz')
ax[0][0].set_ylim(40, 120)
ax[0][0].set_xlim(0, 7.5)
ax[0][1].set_title('Output Spectrum')
ax[0][1].set_ylabel('Magnitude dB')
ax[0][1].set_xlabel('Frequency kHz')
ax[0][1].set_ylim(40, 120)
ax[0][1].set_xlim(0, 7.5)
ax2 = ax[0][1].twinx()
ax2.set_ylim(0, 1)
freq_scale = np.linspace(0, 7.5, 64)
l0 = ax[0][0].plot(freq_scale, 20*np.log10(plot_magnitudes[0]), label="Input Magnitude")
l1 = ax[0][0].plot(freq_scale, 20*np.log10(plot_signal_level[0]), label="Signal Estimate")
l2 = ax[0][0].plot(freq_scale, 20*np.log10(plot_noise[0]), label="Noise Estimate")
l3 = ax[0][1].plot(freq_scale, 20*np.log10(plot_output[0]), label="Output Magnitude")
l4 = ax2.plot(freq_scale, plot_gain[0], color='orange', label="Gain")

log_magnitudes = np.log10(np.array(plot_magnitudes))
scale_min = np.min(np.array(log_magnitudes))
scale_max = np.max(np.array(log_magnitudes))
log_outputs = np.nan_to_num(np.log10(np.array(plot_output)), neginf=scale_min)

animated_waterfall = np.zeros((num_frames, 64))
animated_waterfall[0] = log_magnitudes[0]
wfplot = ax[1][0].imshow(animated_waterfall, aspect="auto", vmin=scale_min, vmax=scale_max)

animated_waterfall_clean = np.zeros((num_frames, 64))
animated_waterfall_clean[0] = log_outputs[0]
wfcplot = ax[1][1].imshow(animated_waterfall_clean, aspect="auto", vmin=scale_min, vmax=scale_max)

ax[0][0].legend()
ax[0][1].legend()
ax2.legend()

plt.tight_layout(pad=1.08, h_pad=None, w_pad=None, rect=None)


def update(frame):

  l0[0].set_ydata(20*np.log10(plot_magnitudes[frame]))
  l1[0].set_ydata(20*np.log10(plot_signal_level[frame]))
  l2[0].set_ydata(20*np.log10(plot_noise[frame]))
  output_signal = np.nan_to_num(20*np.log10(plot_output[frame]), neginf=-100)
  l3[0].set_ydata(output_signal)
  l4[0].set_ydata(plot_gain[frame])
  animated_waterfall = np.zeros((num_frames, 64))
  for i in range(frame):
    animated_waterfall[i] = log_magnitudes[i]
  wfplot.set_array(animated_waterfall)
  for i in range(frame):
    animated_waterfall[i] = log_outputs[i]
  wfcplot.set_array(animated_waterfall)

ani = animation.FuncAnimation(fig=fig, func=update, frames=len(plot_magnitudes), interval=128/15000)
#ani.save("noise_reduction.mp4", writer="ffmpeg", fps=15000/128, dpi=my_dpi)
plt.show()

