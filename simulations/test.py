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
  best_divider = 10000
  for system_clock in vsc:
    divider = round((system_clock*2)/desired_frequency)
    actual_frequency = (system_clock*2)/divider
    if abs(actual_frequency - desired_frequency) < abs(best_frequency - desired_frequency):
      best_frequency = actual_frequency
      best_divider = divider
      best_system_clock = system_clock

  print(desired_frequency, best_frequency, best_system_clock, best_divider)
  return best_frequency

         
frequency = 1e6
frequencies = []
errors_pico2 = []
vsc = valid_system_clocks(125e6, 150e6)
while frequency < 29e6:
  frequencies.append(frequency)
  best_frequency = best_clock_frequency(frequency, vsc) 
  errors_pico2.append(frequency-best_frequency)
  frequency += 0.01e6

print(max(errors_pico2))
print(min(errors_pico2))

from matplotlib import pyplot as plt
plt.title("Best NCO Frequency")
plt.xlabel("Tuned Frequency MHz")
plt.ylabel("Distance from Nearest kHz")
plt.plot(np.array(frequencies)/1e6, np.array(errors_pico2)/1e3, label="pico2")
plt.legend()
plt.show()
