import sys
import serial
import serial.tools.list_ports
import struct
import os
import time
from channel_to_words import words_to_channel


if len(sys.argv) < 2 or "-h" in sys.argv or "--help" in sys.argv:
  print("Download Memory Contents from Pi Pico Rx")
  print("")
  print("Usage: python dowload_memory.py [filename]")
  sys.exit(-1)
filename = sys.argv[1]


#prompt user for port to connect to
if os.path.exists(filename):
  print("%s exists. Replace? Y/N"%filename)
  idx = input()
  if idx.strip().upper() != "Y":
    sys.exit(-1)

print("1. Connect USB cable to Pico Rx")
print("2. When ready press any key")
idx = input()

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

#send csv file to pico via USB
with serial.Serial(port, 12000000, rtscts=1, timeout=1) as ser:

    #clear any data in buffer
    while ser.in_waiting:
      ser.read(ser.in_waiting)
    time.sleep(1)
    while ser.in_waiting:
      ser.read(ser.in_waiting)

    with open(filename, 'w') as output_file:
      output_file.write("Title           , Frequency, Band Start, Band End, Mode, AGC Speed, Frequency Step, bandwdth\n")
      cmd = "ZDN%03x;"%0
      ser.write(bytes(cmd, "utf8"))
      cmd = "ZDN%03x;"%1
      ser.write(bytes(cmd, "utf8"))
      for channel_number in range(512):
        print("Fetching Channel:", channel_number)
        cmd = "ZDN%03x;"%(channel_number+2)
        ser.write(bytes(cmd, "utf8"))
        data = ser.read(135)
        data = data[6:].decode("utf8")
        data = [int(data[i*8:i*8+8], 16) for i in range(16)]
        name, frequency, min_frequency, max_frequency, mode, agc_speed, step, bandwidth = words_to_channel(data)
        csvline = "%s %u %u %u %s %s %s %s\n"%(name, frequency, min_frequency, max_frequency, mode, agc_speed, step, bandwidth)
        output_file.write(csvline)

