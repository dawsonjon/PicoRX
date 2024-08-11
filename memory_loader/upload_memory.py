import sys
import serial
import serial.tools.list_ports
import struct


def read_csv(filename):
  channels = []
  with open(filename) as inf:
    for line in inf:
      line = line.strip()
      line = line.split(",")
      line = [i.strip() for i in line]
      channels.append(line)
  return channels

def pack(string):
    return (ord(string[0]) << 24) + (ord(string[1]) << 16) + (ord(string[2]) << 8) + ord(string[3])

def convert_channel_to_hex(channel):
  name, frequency, min_frequency, max_frequency, mode, agc_speed, step = channel

  if len(name) < 16:
    name += " " * (16-len(name))

  modes = { "AM" :0, "LSB":1, "USB":2, "NFM":3, "CW" :4 }
  agc_speeds = {"FAST": 0, "NORMAL": 1, "SLOW": 2, "VERY SLOW": 3}
  steps = { "10Hz": 0, "50Hz": 1, "100Hz": 2, "1kHz": 3, "5kHz": 4, "10kHz": 5, "12.5kHz": 6, "25kHz": 7, "50kHz": 8, "100kHz": 9,}

  data = [
    int(frequency)&0xffffffff,     #0
    modes[mode],                   #1
    agc_speeds[agc_speed],         #2
    steps[step],                   #3
    int(max_frequency)&0xffffffff, #4
    int(min_frequency)&0xffffffff, #5
    pack(name[0:4]),               #6
    pack(name[4:8]),               #7
    pack(name[8:12]),              #8
    pack(name[12:16]),             #9
    0xffffffff,                    #a
    0xffffffff,                    #b
    0xffffffff,                    #c
    0xffffffff,                    #d
    0xffffffff,                    #e
    0xffffffff,                    #f
  ]
  return data

def read_memory(filename):
  data = read_csv(filename)[1:]
  data = [convert_channel_to_hex(i) for i in data]
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
print("2. Select *USB Memory Upload* menu item")
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
        for location in channel:
          ser.write(bytes("%x\n"%(location), "utf8"))
          ser.readline()
      ser.write(bytes("q\n", "utf8"))

