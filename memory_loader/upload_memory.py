import sys
import serial
import serial.tools.list_ports
import struct
from channel_to_words import channel_to_words


def read_csv(filename):
  channels = []
  with open(filename) as inf:
    for line in inf:
      line = line.strip()
      line = line.split(",")
      line = [i.strip() for i in line]
      channels.append(line)
  return channels

def read_memory(filename):
  data = read_csv(filename)[1:]
  data = [channel_to_words(*i) for i in data]
  data = data[:512]
  return data
    

if len(sys.argv) < 2 or "-h" in sys.argv or "--help" in sys.argv:
  print("Upload Memory Contents to Pi Pico Rx")
  print("")
  print("Usage: python upload_memory.py [filename]")
  sys.exit(-1)
filename = sys.argv[1]


#prompt user for port to connect to
print("!! This will overwrite the memory contents of the pi-pico Rx !!")
print("Are you sure? Y/N")
idx = input()
if idx.strip().upper() != "Y":
  sys.exit(-1)

print("1. Connect USB cable to Pico Rx")
print("2. Select *HW Config -> USB Upload -> Memory* menu item")
print("3. When ready press any key")
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
buffer = read_memory(filename)
with serial.Serial(port, 12000000, rtscts=1) as ser:

    #clear any data in buffer
    while ser.in_waiting:
      ser.read(ser.in_waiting)

    with open(filename, 'rb') as input_file:
      for channel in buffer:
        i=0
        for location in channel:
          i+=1
          ser.write(bytes("%x\n"%(location), "utf8"))
      ser.write(bytes("q\n", "utf8"))

