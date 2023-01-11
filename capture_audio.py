import serial
import numpy as np
from matplotlib import pyplot as plt
import serial.tools.list_ports
import struct
import sys

#get a list of available serial ports
print("Available Ports", file=sys.stderr)
portinfo = dict(enumerate(serial.tools.list_ports.comports()))
for idx, info in portinfo.items():
    port, desc, hwid = info
    print(idx, "{}: {} [{}]".format(port, desc, hwid))

#prompt user for port to connect to
while 1:
    print("Select COM port >", file=sys.stderr)
    idx = input()
    port = portinfo.get(int(idx), None)
    if port is not None:
        port = port[0]
        break

plot_data = False
with serial.Serial(port, 12000000) as ser:

    while True:
      data = ser.read(81920)
      sys.stdout.buffer.write(data)


      if plot_data:
        dt = np.dtype("int16")
        dt = dt.newbyteorder('<')
        data = np.frombuffer(data, dtype=dt)

        plt.plot(data)
        plt.show()

        plt.plot(20*np.log10(np.fft.fftshift(abs(np.fft.fft(data)))))
        plt.show()



