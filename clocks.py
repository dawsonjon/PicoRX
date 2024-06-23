from math import ceil, floor


minimum_frequency = 125e6
maximum_frequency = 133e6
valid_system_clocks = {}

#generate a table of all system frequencies in the valid range
for refdiv in range(1, 2):
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

def best_clock_frequency(desired_frequency):
  best_frequency = 1
  for system_clock in valid_system_clocks:
    best_divider = round(256 * system_clock/desired_frequency) / 256
    actual_frequency = system_clock/best_divider
    if abs(actual_frequency - desired_frequency) < abs(best_frequency - desired_frequency):
      best_frequency = actual_frequency
  return best_frequency

         
frequency = 1e6
frequencies = []
errors = []
while frequency < 29e6:
  best_frequency = best_clock_frequency(frequency*4) 
  #print(frequency, best_frequency/4, frequency-best_frequency/4)
  frequencies.append(frequency)
  errors.append(frequency-best_frequency/4)
  frequency += 0.001e6

from matplotlib import pyplot as plt
plt.plot(range(len(ratios)), ratios)
plt.show()
plt.plot(frequencies, errors)
plt.show()
        
