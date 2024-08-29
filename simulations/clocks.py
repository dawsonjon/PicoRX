from math import ceil, floor
import numpy as np


def valid_system_clocks(minimum_frequency, maximum_frequency):
  valid_system_clocks = {}

  #generate a table of all system frequencies in the valid range
  for refdiv in reversed(range(1, 2)):
    reffreq = 12e6/refdiv
    for fbdiv in range(16, 320):
      vco_freq = reffreq*fbdiv
      if vco_freq > 1600e6:
        continue
      if vco_freq < 750e6:
        continue
      for postdiv1 in range(1, 8):
        for postdiv2 in range(1, 8):
          f = vco_freq/(postdiv1*postdiv2)
          if f > maximum_frequency:
            continue
          if f < minimum_frequency:
            continue
          valid_system_clocks[int(f)] = (refdiv, fbdiv, postdiv1, postdiv2)

  print("valid_system_clocks")
  ratios = []
  for i, f in enumerate(sorted(valid_system_clocks.keys())):
    settings = valid_system_clocks[f]
    print("{%u, %u, %u, %u, %u},"%((f,)+settings))
    ratios.append(f/minimum_frequency)

  return valid_system_clocks

def best_clock_frequency(desired_frequency, vsc):
  best_frequency = 1
  for system_clock in vsc:
    best_divider = round(256 * system_clock/desired_frequency) / 256
    actual_frequency = system_clock/best_divider
    if abs(actual_frequency - desired_frequency) < abs(best_frequency - desired_frequency):
      best_frequency = actual_frequency
  return best_frequency

         
frequency = 1e6
frequencies = []
errors_new = []
vsc = valid_system_clocks(125e6, 133.1e6)
while frequency < 29e6:
  best_frequency = best_clock_frequency(frequency*4, vsc) 
  frequencies.append(frequency)
  errors_new.append(frequency-best_frequency/4)
  frequency += 0.001e6

frequency = 1e6
errors_pico2 = []
vsc = valid_system_clocks(125e6, 150.1e6)
while frequency < 29e6:
  best_frequency = best_clock_frequency(frequency*4, vsc) 
  errors_pico2.append(frequency-best_frequency/4)
  frequency += 0.001e6

frequency = 1e6
errors_old = []
vsc = valid_system_clocks(125e6, 125.1e6)
while frequency < 29e6:
  best_frequency = best_clock_frequency(frequency*4, vsc) 
  errors_old.append(frequency-best_frequency/4)
  frequency += 0.001e6

print(max(errors_pico2))
print(min(errors_pico2))
print(max(errors_new))
print(min(errors_new))
print(max(errors_old))
print(min(errors_old))

from matplotlib import pyplot as plt
plt.title("Best NCO Frequency")
plt.xlabel("Tuned Frequency MHz")
plt.ylabel("Distance from Nearest kHz")
plt.plot(np.array(frequencies)/1e6, np.array(errors_old)/1e3, label="old design")
plt.plot(np.array(frequencies)/1e6, np.array(errors_new)/1e3, label="new design")
plt.plot(np.array(frequencies)/1e6, np.array(errors_pico2)/1e3, label="pico2")
plt.legend()
plt.show()
