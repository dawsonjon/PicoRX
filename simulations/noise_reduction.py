import numpy as np
from matplotlib import pyplot as plt


Fs = 15000


with open("test.wav", "rb") as inf:
  with open("result.wav", "wb") as outf:

    plot_bin = []
    noise_bin = []
    signal_bin = []
    gain_bin = []
    snr_bin = []
    image = []
    image_clean = []


    input_overlap = np.zeros(64)
    output_overlap = np.zeros(64)
    noise_level = np.ones(128)*1000000
    signal_level = np.zeros(128)
    gain = np.zeros(128)
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

      #output_frame = input_frame
      magnitudes = np.abs(np.fft.fftshift(input_frame))
      signal_level = signal_level.astype("int") + ((magnitudes.astype("int") - signal_level.astype("int")) >> 3)
      #signal_level = (0.9 * signal_level) + (0.1*magnitudes)
      noise_level[signal_level <= noise_level] = signal_level[signal_level <= noise_level]
      noise_level[signal_level > noise_level] = np.minimum(noise_level + (noise_level.astype("int") >> 10), signal_level)[signal_level > noise_level]
      #noise_level[signal_level > noise_level] = np.minimum(noise_level + 1, signal_level)[signal_level > noise_level]

      #noise frequency smoothing
      if False:
        smoothed_noise_level = (np.convolve(noise_level, np.ones(4))/4)[2:-1]
      else:
        smoothed_noise_level = noise_level

      #time smoothing
      if False:
        gain = np.clip(0.1*(1-(3.0*smoothed_noise_level/signal_level))+0.9*gain , 0, 1)
      else:
        #gain = np.clip((1-(6.0*smoothed_noise_level*smoothed_noise_level/(signal_level*signal_level))), 0, 1)
        #gain = np.clip((1-(3.0*smoothed_noise_level/signal_level)), 0, 1)**2
        gain = np.clip((1-(3.2*smoothed_noise_level/signal_level)), 0, 1)*2

      #gain = gain > 0.5
      
      #plt.plot(magnitudes)
      #plt.plot(noise_level)
      #plt.plot(smoothed_noise_level)
      #noise_level = smoothed_noise_level
      #plt.ylim([0, 100000])
      #plt.show()

      voice_present = np.sum(signal_level > (noise_level * 2)) > 20

      output_frame = input_frame * np.fft.fftshift(gain)
      image_clean.append(np.log(magnitudes*gain))
      image.append(np.log(magnitudes))

      #plot gains for 1 bin
      plot_bin.append(magnitudes[74])
      noise_bin.append(noise_level[74])
      signal_bin.append(signal_level[74])
      gain_bin.append(gain[74])
      snr_bin.append(voice_present)

      #add overlaping output segments then convert to frequency domain
      output_frame = np.fft.ifft(output_frame)
      output_frame = np.sqrt(np.hanning(len(output_frame))) * output_frame #analysis window
      audio = output_frame[:64] + output_overlap
      output_overlap = output_frame[64:]
      audio = audio.real
      audio = audio.astype("int16")
      outf.write(audio.tobytes())
    

if True:
  f, ax = plt.subplots(2,1)
  time = 64*np.arange(len(plot_bin))/Fs
  ax[0].plot(time, 20*np.log10(np.array(plot_bin)))
  ax[0].plot(time, 20*np.log10(np.array(noise_bin)))
  ax[0].plot(time, 20*np.log10(np.array(signal_bin)))
  ax[1].plot(time, np.array(gain_bin))
  #plt.plot(time, np.array(snr_bin)*100000)
  plt.show()

  f, ax = plt.subplots(1,2)
  f.figsize=(300, 30)
  ax[0].imshow(image)
  ax[1].imshow(image_clean)
  plt.show()
