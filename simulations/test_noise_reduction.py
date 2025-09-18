import numpy as np
from matplotlib import pyplot as plt
from subprocess import run, PIPE, Popen


#build and run test harness
run(["g++", "-DSIMULATION=true", "../utils.cpp", "../noise_reduction.cpp", "noise_reduction_test.cpp", "-o", "noise_reduction_test"])
uut = Popen("./noise_reduction_test", stdin=PIPE, stdout=PIPE)


Fs = 15000

with open("test.wav", "rb") as inf:
  with open("result.wav", "wb") as outf:

    input_overlap = np.zeros(64)
    output_overlap = np.zeros(64)
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

      data = " ".join(["%u %u"%(int(i//128), int(q//128)) for i, q in zip(input_frame.real, input_frame.imag)])
      data += "\n"
      uut.stdin.write(bytes(data, "utf8"))
      uut.stdin.flush()

      data = uut.stdout.readline()
      data = np.array([int(i) for i in data.split()])
      output_frame = 128 * (data[0::4]+(1.0j*data[1::4]))
      noise_estimate = data[2::4]
      signal_estimate = data[3::4]

      #plt.plot(signal_estimate)
      #plt.plot(noise_estimate)
      #plt.show()

      #add overlaping output segments then convert to frequency domain
      output_frame = np.fft.ifft(output_frame)
      output_frame = np.sqrt(np.hanning(len(output_frame))) * output_frame #analysis window
      audio = output_frame[:64] + output_overlap
      output_overlap = output_frame[64:]
      audio = audio.real
      audio = audio.astype("int16")
      outf.write(audio.tobytes())
    

if False:
  f, ax = plt.subplots(2,1)
  time = 64*np.arange(len(plot_bin))/Fs
  ax[0].plot(time, 20*np.log10(np.array(plot_bin)))
  ax[0].plot(time, 20*np.log10(np.array(noise_bin)))
  ax[0].plot(time, 20*np.log10(np.array(signal_bin)))
  #ax[1].plot(time, np.array(gain_bin))
  #plt.plot(time, np.array(snr_bin)*100000)
  plt.show()

  f, ax = plt.subplots(1,2)
  f.figsize=(300, 30)
  ax[0].imshow(image)
  ax[1].imshow(image_clean)
  plt.show()
