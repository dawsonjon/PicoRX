#!/usr/bin/env python
import struct

#define idx_frequency 0
#define idx_mode 1
#define idx_agc_speed 2
#define idx_step 3
#define idx_squelch 4
#define idx_volume 5
#define idx_max_frequency 6
#define idx_min_frequency 7
#define idx_cw_sidetone 8
#define idx_cw_speed 9

modes = {
  "AM" :0,
  "AMS":1,
  "LSB":2,
  "USB":3,
  "NFM":4,
  "CW" :5
}
agc_speeds = {"FAST": 0, "NORMAL": 1, "SLOW": 2, "VERY SLOW": 3}
steps = {
    "10Hz": 0,
    "50Hz": 1,
    "100Hz": 2,
    "1kHz": 3,
    "5kHz": 4,
    "9kHz": 5,
    "10kHz": 6,
    "12.5kHz": 7,
    "25kHz": 8,
    "50kHz": 9,
    "100kHz": 10,
}

def pack(string):
    return (ord(string[0]) << 24) + (ord(string[1]) << 16) + (ord(string[2]) << 8) + ord(string[3])

num_channels = 512
channel_size = 16
class Memory:
  def __init__(self):
    self.memory = []

  def add(self, name, frequency, min_frequency, max_frequency, mode, agc_speed, step):
      assert len(name) == 16

      #split frequency into bytes
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
      self.memory.append(data)

  def generate_c_header(self):
      while len(self.memory) < num_channels:
        self.memory.append([0xffffffff for i in range(channel_size)])
      data = ",\n".join(["{"+", ".join([str(i) for i in data])+"}" for data in self.memory])
      buffer = """#ifndef __memory__
#define __memory__

#include <pico/platform.h>
static const uint32_t num_chans = %s;
static const uint32_t chan_size = %s;
static const uint32_t __in_flash() __attribute__((aligned(4096))) radio_memory[%s][%s] = {
%s
};
#endif"""%(num_channels, channel_size, num_channels, channel_size, data)
      with open("memory.h", "w") as outf:
        outf.write(buffer)

mem = Memory()
mem.add("All Bands       ", 198000,   0,        30000000, "AM", "VERY SLOW", "1kHz")
mem.add("MW Broadcast    ", 1413000,  531000,   1602000,  "AM", "VERY SLOW", "9kHz")
mem.add("LW Broadcast    ", 198000,   153000,   279000,   "AM", "VERY SLOW", "1kHz")
mem.add("SW 120m         ", 2300000,  2300000,  2495000,  "AM", "VERY SLOW", "1kHz")
mem.add("SW 90m          ", 3200000,  3200000,  3400000,  "AM", "VERY SLOW", "1kHz")
mem.add("SW 75m          ", 3900000,  3900000,  4000000,  "AM", "VERY SLOW", "1kHz")
mem.add("SW 60m          ", 4750000,  4750000,  4995000,  "AM", "VERY SLOW", "1kHz")
mem.add("SW 49m          ", 5900000,  5900000,  6200000,  "AM", "VERY SLOW", "1kHz")
mem.add("SW 41m          ", 7200000,  7200000,  7450000,  "AM", "VERY SLOW", "1kHz")
mem.add("SW 31m          ", 9400000,  9400000,  9900000,  "AM", "VERY SLOW", "1kHz")
mem.add("SW 25m          ", 11600000, 11600000, 12100000, "AM", "VERY SLOW", "1kHz")
mem.add("SW 22m          ", 13570000, 13570000, 13870000, "AM", "VERY SLOW", "1kHz")
mem.add("SW 19m          ", 15100000, 15100000, 15800000, "AM", "VERY SLOW", "1kHz")
mem.add("SW 16m          ", 17480000, 17480000, 17900000, "AM", "VERY SLOW", "1kHz")
mem.add("SW 15m          ", 18900000, 18900000, 19020000, "AM", "VERY SLOW", "1kHz")
mem.add("SW 13m          ", 21450000, 21450000, 21850000, "AM", "VERY SLOW", "1kHz")
mem.add("SW 11m          ", 25670000, 25670000, 26100000, "AM", "VERY SLOW", "1kHz")
mem.add("10m SSB         ", 28300000, 28300000, 29000000, "USB", "SLOW", "100Hz")
mem.add("12m SSB         ", 24931000, 24931000, 24990000, "USB", "SLOW", "100Hz")
mem.add("15m SSB         ", 21151000, 21151000, 21450000, "USB", "SLOW", "100Hz")
mem.add("17m SSB         ", 18111000, 18111000, 18168000, "USB", "SLOW", "100Hz")
mem.add("20m SSB         ", 14101000, 14101000, 14350000, "USB", "SLOW", "100Hz")
mem.add("40m SSB         ", 7060000,  7060000,  7200000,  "LSB", "SLOW", "100Hz")
mem.add("80m SSB         ", 3620000,  3620000,  3800000,  "LSB", "SLOW", "100Hz")
mem.add("160m SSB        ", 1843000,  1843000,  2000000,  "LSB", "SLOW", "100Hz")
mem.add("10m CW          ", 28000000, 28000000, 28070000, "CW", "SLOW", "10Hz")
mem.add("12m CW          ", 24890000, 24890000, 24915000, "CW", "SLOW", "10Hz")
mem.add("15m CW          ", 21000000, 21000000, 21070000, "CW", "SLOW", "10Hz")
mem.add("17m CW          ", 18068000, 18068000, 18095000, "CW", "SLOW", "10Hz")
mem.add("20m CW          ", 14000000, 14000000, 14099000, "CW", "SLOW", "10Hz")
mem.add("30m CW          ", 10100000, 10100000, 10130000, "CW", "SLOW", "10Hz")
mem.add("40m CW          ", 7000000,  7000000,  7060000,  "CW", "SLOW", "10Hz")
mem.add("80m CW          ", 3500000,  3500000,  3620000,  "CW", "SLOW", "10Hz")
mem.add("160m CW         ", 1810000,  1810000,  1843000,  "CW", "SLOW", "10Hz")
mem.add("10m PSK         ", 28120000, 28070000, 28190000, "USB", "SLOW", "100Hz")
mem.add("12m PSK         ", 24920000, 24195000, 24929000, "USB", "SLOW", "100Hz")
mem.add("15m PSK         ", 21080000, 21070000, 21149000, "USB", "SLOW", "100Hz")
mem.add("17m PSK         ", 18097000, 18095000, 18109000, "USB", "SLOW", "100Hz")
mem.add("20m PSK         ", 14070000, 14070000, 14099000, "USB", "SLOW", "100Hz")
mem.add("30m PSK         ", 10141000, 10130000, 10150000, "USB", "SLOW", "100Hz")
mem.add("40m PSK         ", 7040000,  7000000,  7060000,  "USB", "SLOW", "100Hz")
mem.add("80m PSK         ", 3580000,  3500000,  3620000,  "USB", "SLOW", "100Hz")
mem.add("160m PSK        ", 1830000,  1810000,  1843000,  "USB", "SLOW", "100Hz")
mem.add("10m FT8         ", 28074000, 28070000, 28190000, "USB", "SLOW", "100Hz")
mem.add("12m FT8         ", 24915000, 24195000, 24929000, "USB", "SLOW", "100Hz")
mem.add("15m FT8         ", 21074000, 21070000, 21149000, "USB", "SLOW", "100Hz")
mem.add("17m FT8         ", 18100000, 18095000, 18109000, "USB", "SLOW", "100Hz")
mem.add("20m FT8         ", 14074000, 14070000, 14099000, "USB", "SLOW", "100Hz")
mem.add("30m FT8         ", 10136000, 10130000, 10150000, "USB", "SLOW", "100Hz")
mem.add("40m FT8         ", 7074000,  7040000,  7060000,  "USB", "SLOW", "100Hz")
mem.add("80m FT8         ", 3573000,  3570000,  3600000,  "USB", "SLOW", "100Hz")
mem.add("160m FT8        ", 1840000,  1810000,  1843000,  "USB", "SLOW", "100Hz")
mem.add("10m FT4         ", 28180000, 28070000, 28190000, "USB", "SLOW", "100Hz")
mem.add("12m FT4         ", 24919000, 24195000, 24929000, "USB", "SLOW", "100Hz")
mem.add("15m FT4         ", 21140000, 21070000, 21149000, "USB", "SLOW", "100Hz")
mem.add("17m FT4         ", 18104000, 18095000, 18109000, "USB", "SLOW", "100Hz")
mem.add("20m FT4         ", 14140000, 14070000, 14099000, "USB", "SLOW", "100Hz")
mem.add("30m FT4         ", 10140000, 10130000, 10150000, "USB", "SLOW", "100Hz")
mem.add("40m FT4         ", 7090000,  7040000,  7060000,  "USB", "SLOW", "100Hz")
mem.add("80m FT4         ", 3595000,  3570000,  3600000,  "USB", "SLOW", "100Hz")
mem.add("10m WSPR        ", 28124600, 28070000, 28190000, "USB", "SLOW", "100Hz")
mem.add("12m WSPR        ", 24924600, 24195000, 24929000, "USB", "SLOW", "100Hz")
mem.add("15m WSPR        ", 21094600, 21070000, 21149000, "USB", "SLOW", "100Hz")
mem.add("17m WSPR        ", 18104600, 18095000, 18109000, "USB", "SLOW", "100Hz")
mem.add("20m WSPR        ", 14095600, 14070000, 14099000, "USB", "SLOW", "100Hz")
mem.add("30m WSPR        ", 10138700, 10130000, 10150000, "USB", "SLOW", "100Hz")
mem.add("40m WSPR        ", 7038600,  7040000,  7060000,  "USB", "SLOW", "100Hz")
mem.add("80m WSPR        ", 3568600,  3570000,  3600000,  "USB", "SLOW", "100Hz")
mem.add("160m WSPR       ", 1836600,  1810000,  1843000,  "USB", "SLOW", "100Hz")
mem.add("10m RTTY        ", 28080000, 28070000, 28190000, "USB", "SLOW", "100Hz")
mem.add("12m RTTY        ", 24925000, 24195000, 24929000, "USB", "SLOW", "100Hz")
mem.add("15m RTTY        ", 21080000, 21070000, 21149000, "USB", "SLOW", "100Hz")
mem.add("17m RTTY        ", 18106000, 18095000, 18109000, "USB", "SLOW", "100Hz")
mem.add("20m RTTY        ", 14083000, 14070000, 14099000, "USB", "SLOW", "100Hz")
mem.add("30m RTTY        ", 10143000, 10130000, 10150000, "USB", "SLOW", "100Hz")
mem.add("40m RTTY        ", 7043000,  7040000,  7060000,  "USB", "SLOW", "100Hz")
mem.add("80m RTTY        ", 3590000,  3570000,  3600000,  "USB", "SLOW", "100Hz")
mem.add("GFA WEAFAX 1    ", 2616600,  2616600,  2616600,  "USB", "SLOW", "100Hz")
mem.add("GFA WEAFAX 2    ", 4608100,  4608100,  4608100,  "USB", "SLOW", "100Hz")
mem.add("GFA WEAFAX 3    ", 8038100,  8038100,  8038100,  "USB", "SLOW", "100Hz")
mem.add("GFA WEAFAX 4    ", 14434100,  14434100,  14434100,  "USB", "SLOW", "100Hz")
mem.add("GFA WEAFAX 5    ", 18259100,  18259100,  18259100,  "USB", "SLOW", "100Hz")
mem.add("RAF VOLMET      ", 5450000,  5480000,  5730000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON VOLMET  ", 5505000,  5480000,  5730000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON VOLMET 2", 5662000,  5480000,  5730000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON ATC    1", 5598000,  5480000,  5730000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON ATC    2", 5616000,  5480000,  5730000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON ATC    3", 5649000,  5480000,  5730000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON ATC    4", 5658000,  5480000,  5730000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON ATC    5", 8906000,  8815000,  9040000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON ATC    6", 8864000,  8815000,  9040000,  "USB", "SLOW", "10Hz")
mem.add("SHANNON ATC    7", 8879000,  8815000,  9040000,  "USB", "SLOW", "10Hz")
mem.add("KINLOSS RESCUE1 ", 5680000,  5598000,  5505000,  "USB", "SLOW", "10Hz")
mem.add("BBC Word Serv 1 ", 3915000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("BBC Word Serv 2 ", 5890000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("BBC Word Serv 3 ", 12095000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("BBC Word Serv 4 ", 15335000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 1   ", 909000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 2   ", 1530000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 3   ", 4930000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 4   ", 4960000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 5   ", 6080000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 6   ", 9550000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 7   ", 13590000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 8   ", 15580000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("V O America 9   ", 17895000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("RADIO JAPAN 1   ", 5955000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("RADIO JAPAN 2   ", 6104000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("RADIO JAPAN 3   ", 11860000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("RADIO JAPAN 4   ", 11935000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 1", 5960000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 2", 7285000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 3", 7350000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 4", 7415000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 5", 9470000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 6", 9600000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 7", 9675000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 8", 11940000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INTL 9", 11965000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 10", 12015000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 11", 13665000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 12", 13670000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 13", 13760000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 14", 13710000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 15", 15245000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 16", 17490000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 17", 17570000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 18", 17630000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("CHINA RAD INT 19", 17650000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("UNITES NATIONS 1", 9565000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("UNITES NATIONS 2", 17810000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 1       ", 6185000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 2       ", 7230000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 3       ", 7235000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 4       ", 7250000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 5       ", 7360000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 6       ", 9645000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 7       ", 9705000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 8       ", 11620000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 9       ", 11815000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 10      ", 15595000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("VATICAN 11      ", 17790000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz")
mem.add("BBC RADIO 5 Live", 693000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz")
mem.add("BBC Her and worc", 738000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz")
mem.add("855 Sunshine    ", 855000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz")
mem.add("Talk Sport      ",1053000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz")
mem.add("BBC Gloucester  ",1413000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz")
mem.add("BBC Radio 4     ", 198000,   153000,   279000,   "AM", "VERY SLOW", "1kHz")
mem.add("RTE Radio 1     ", 252000,   153000,   279000,   "AM", "VERY SLOW", "1kHz")

#100 - 200 CB frequencies
cb_start = 27601250
cb_stop = 27991250
cb_step = 10000
for channel in range(40):
    mem.add("CB UK %2u        "%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz")

cb_start = 26965000
cb_stop = 27405000
cb_step = 10000
for channel in range(40):
    mem.add("CB CEPT %2u      "%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz")

cb_start = 29100000
cb_stop = 29200000
cb_step = 10000
mem.add("10m FM CALLING  ", cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz")
for channel in range(40):
    mem.add("10m FM SIMPLEX%2u"%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz")

mem.generate_c_header()
