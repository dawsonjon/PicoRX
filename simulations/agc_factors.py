from matplotlib import pyplot as plt
import numpy as np

start_fs = 500e3
decimate = 32

fs = start_fs/(decimate)
threshold = 0.9
factor_l = 14
factor_s = 12
factor_m = 11
factor_f = 10
attack_factor = 2

t=0
magf = 0
magm = 0
mags = 0
magl = 0
audio = 1<<15
magsf = []
magsm = []
magss = []
magsl = []
ts = []
audio <<= 16
while (audio*threshold) > magl:
  magsf.append(magf)
  magsm.append(magm)
  magss.append(mags)
  magsl.append(magl)
  ts.append(t)
  magl += (audio - magl) >> factor_l
  mags += (audio - mags) >> factor_s
  magm += (audio - magm) >> factor_m
  magf += (audio - magf) >> factor_f
  t += 1/fs

decayts = ts
fast_decay = np.interp(threshold, np.array(magsf)/audio, decayts)
medium_decay = np.interp(threshold, np.array(magsm)/audio, decayts)
slow_decay = np.interp(threshold, np.array(magss)/audio, decayts)
long_decay = np.interp(threshold, np.array(magsl)/audio, decayts)

t=0
audio = 1<<15
mag = audio
magsa = []
ts = []
audio <<= 16
mag <<= 16
while mag > (audio*0.1):
  magsa.append(mag)
  ts.append(t)
  mag -= mag >> attack_factor
  t += 1/fs


attack = ts[-1]

print("input fs=%f Hz"%start_fs)
print("decimation=%u x 2"%decimate)
print("fs=%f Hz"%fs)
print("Setting Decay Time(s) Factor Attack Time(s) Factor  Hang  Timer")
print("======= ============= ====== ============== ======  ====  =====")
print("fast        %0.3f          %2u       %0.3f     %2u    0.1s   %u"%(fast_decay, factor_f, attack, attack_factor, fs/10))
print("medium      %0.3f          %2u       %0.3f     %2u    0.25s  %u"%(medium_decay, factor_m, attack, attack_factor, fs/4))
print("slow        %0.3f          %2u       %0.3f     %2u    1s     %u"%(slow_decay, factor_s, attack, attack_factor, fs))
print("long        %0.3f          %2u       %0.3f     %2u    2s     %u"%(long_decay, factor_l, attack, attack_factor, fs*2))

plt.plot(decayts, np.array(magsf)/audio)
plt.plot(decayts, np.array(magsm)/audio)
plt.plot(decayts, np.array(magss)/audio)
plt.plot(decayts, np.array(magsl)/audio)
plt.plot(decayts, np.ones(len(magsf))*threshold)
plt.plot(fast_decay, threshold, "rx")
plt.plot(medium_decay, threshold, "rx")
plt.plot(slow_decay, threshold, "rx")
plt.plot(long_decay, threshold, "rx")
plt.show()

plt.plot(ts, np.array(magsa)/audio)
plt.plot(ts, np.ones(len(magsa))*0.1)
plt.plot(attack, 0.1, "rx")
plt.show()
