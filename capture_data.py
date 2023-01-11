import serial
import numpy as np
from matplotlib import pyplot as plt
import serial.tools.list_ports

#get a list of available serial ports
print("Available Ports")
portinfo = dict(enumerate(serial.tools.list_ports.comports()))
for idx, info in portinfo.items():
    port, desc, hwid = info
    print(idx, "{}: {} [{}]".format(port, desc, hwid))

#prompt user for port to connect to
while 1:
    print("Select COM port >")
    idx = input()
    port = portinfo.get(int(idx), None)
    if port is not None:
        port = port[0]
        break

with serial.Serial('/dev/ttyACM0', 12000000, rtscts=1) as ser:
    data = ser.read(50000).decode("utf8")
    data = data.split("capture:")[1]
    data = data.split("time")[0].strip()
    data = [int(i) for i in data.split()]
    i = data[:len(data)//2]
    q = data[len(data)//2:]
    data = np.array(i)+1.0j*np.array(q)
    plt.plot(data.real)
    plt.plot(data.imag)
    plt.show()
    
    plt.plot(np.linspace(-25, 25, len(data)), 20*np.log10(np.fft.fftshift(abs(np.fft.fft(data)))))
    plt.show()

