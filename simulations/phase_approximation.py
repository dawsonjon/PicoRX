from math import sin, cos, pi, atan2
from matplotlib import pyplot as plt

def phase(i, q):
   absi = abs(i)
   if (q>=0):
      r = ((q - absi)<<13) / (q + absi);
      angle = 8192 - r;
   else:
      r = ((q + absi)<<13) / (absi - q);
      angle = 24576 - r;

   print(r)

   if (i < 0):
     return -angle
   else:
     return angle


stimulus = [(int(round(sin(2*pi*i/100)*32767)), int(round(cos(2*pi*i/100)*32767))) for i in range(100)]
response = [phase(i, q) for i, q in stimulus]
print(response)

plt.plot(response)
plt.plot([32768 * atan2(i, q)/pi for i, q in stimulus])
plt.show()
