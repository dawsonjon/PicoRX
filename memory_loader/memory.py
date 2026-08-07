#!/usr/bin/env python
import struct
from channel_to_words import channel_to_words

num_channels = 512
channel_size = 16
class Memory:
  def __init__(self):
    self.memory = []

  def add(self, data):
      self.memory.append(data)

  def generate_c_header(self):
      while len(self.memory) < num_channels:
        self.add(channel_to_words("BLANK           ", 0,   0,        30000000, "AM", "VERY SLOW", "1kHz", "Normal"))

      data = ",\n".join(["{"+", ".join([str(i) for i in data])+"}" for data in self.memory])
      buffer = """#include "memory.h"
#include <pico.h>
const uint32_t __in_flash() __attribute__((aligned(4096))) radio_memory[%s][%s] = {
%s
};
"""%(num_channels, channel_size, data)
      with open("src/memory.cpp", "w") as outf:
        outf.write(buffer)

mem = Memory()
mem.add(channel_to_words("All Bands       ", 198000,   0,        30000000, "AM", "VERY SLOW", "1kHz", "Normal"))
mem.add(channel_to_words("MW Broadcast    ", 1413000,  531000,   1602000,  "AM", "VERY SLOW", "9kHz", "Normal"))
mem.add(channel_to_words("LW Broadcast    ", 198000,   153000,   279000,   "AM", "VERY SLOW", "1kHz", "Normal"))
mem.add(channel_to_words("SW 120m         ", 2300000,  2300000,  2495000,  "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 90m          ", 3200000,  3200000,  3400000,  "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 75m          ", 3900000,  3900000,  4000000,  "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 60m          ", 4750000,  4750000,  4995000,  "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 49m          ", 5900000,  5900000,  6200000,  "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 41m          ", 7200000,  7200000,  7450000,  "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 31m          ", 9400000,  9400000,  9900000,  "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 25m          ", 11600000, 11600000, 12100000, "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 22m          ", 13570000, 13570000, 13870000, "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 19m          ", 15100000, 15100000, 15800000, "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 16m          ", 17480000, 17480000, 17900000, "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 15m          ", 18900000, 18900000, 19020000, "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 13m          ", 21450000, 21450000, 21850000, "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("SW 11m          ", 25670000, 25670000, 26100000, "AM", "VERY SLOW", "5kHz", "Normal"))
mem.add(channel_to_words("10m SSB         ", 28300000, 28300000, 29000000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("12m SSB         ", 24931000, 24931000, 24990000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("15m SSB         ", 21151000, 21151000, 21450000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("17m SSB         ", 18111000, 18111000, 18168000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("20m SSB         ", 14101000, 14101000, 14350000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("40m SSB         ", 7060000,  7060000,  7200000,  "LSB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("80m SSB         ", 3620000,  3620000,  3800000,  "LSB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("160m SSB        ", 1843000,  1843000,  2000000,  "LSB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("10m CW          ", 28000000, 28000000, 28070000, "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("12m CW          ", 24890000, 24890000, 24915000, "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("15m CW          ", 21000000, 21000000, 21070000, "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("17m CW          ", 18068000, 18068000, 18095000, "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("20m CW          ", 14000000, 14000000, 14099000, "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("30m CW          ", 10100000, 10100000, 10130000, "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("40m CW          ", 7000000,  7000000,  7060000,  "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("80m CW          ", 3500000,  3500000,  3620000,  "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("160m CW         ", 1810000,  1810000,  1843000,  "CW", "SLOW", "50Hz", "Normal"))
mem.add(channel_to_words("20m SSTV        ", 14230000, 14101000, 14350000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("40m SSTV        ", 7033000,  7060000,  7200000,  "LSB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("80m SSTV        ", 3845000,  3620000,  3800000,  "LSB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("10m PSK         ", 28120000, 28070000, 28190000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("12m PSK         ", 24920000, 24195000, 24929000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("15m PSK         ", 21080000, 21070000, 21149000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("17m PSK         ", 18097000, 18095000, 18109000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("20m PSK         ", 14070000, 14070000, 14099000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("30m PSK         ", 10141000, 10130000, 10150000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("40m PSK         ", 7040000,  7000000,  7060000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("80m PSK         ", 3580000,  3500000,  3620000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("160m PSK        ", 1830000,  1810000,  1843000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("10m FT8         ", 28074000, 28070000, 28190000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("12m FT8         ", 24915000, 24195000, 24929000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("15m FT8         ", 21074000, 21070000, 21149000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("17m FT8         ", 18100000, 18095000, 18109000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("20m FT8         ", 14074000, 14070000, 14099000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("30m FT8         ", 10136000, 10130000, 10150000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("40m FT8         ", 7074000,  7040000,  7060000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("80m FT8         ", 3573000,  3570000,  3600000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("160m FT8        ", 1840000,  1810000,  1843000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("10m FT4         ", 28180000, 28070000, 28190000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("12m FT4         ", 24919000, 24195000, 24929000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("15m FT4         ", 21140000, 21070000, 21149000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("17m FT4         ", 18104000, 18095000, 18109000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("20m FT4         ", 14080000, 14070000, 14099000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("30m FT4         ", 10140000, 10130000, 10150000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("40m FT4         ", 7047500,  7040000,  7060000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("80m FT4         ", 3575000,  3570000,  3600000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("10m WSPR        ", 28124600, 28070000, 28190000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("12m WSPR        ", 24924600, 24195000, 24929000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("15m WSPR        ", 21094600, 21070000, 21149000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("17m WSPR        ", 18104600, 18095000, 18109000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("20m WSPR        ", 14095600, 14070000, 14099000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("30m WSPR        ", 10138700, 10130000, 10150000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("40m WSPR        ", 7038600,  7040000,  7060000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("80m WSPR        ", 3568600,  3570000,  3600000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("160m WSPR       ", 1836600,  1810000,  1843000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("10m RTTY        ", 28080000, 28070000, 28190000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("12m RTTY        ", 24925000, 24195000, 24929000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("15m RTTY        ", 21080000, 21070000, 21149000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("17m RTTY        ", 18106000, 18095000, 18109000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("20m RTTY        ", 14083000, 14070000, 14099000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("30m RTTY        ", 10143000, 10130000, 10150000, "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("40m RTTY        ", 7043000,  7040000,  7060000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("80m RTTY        ", 3590000,  3570000,  3600000,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("NAVTEX NATIONAL ", 490000,  3570000,  3600000,  "LSB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("NAVTEX GLOBAL   ", 518000,  3570000,  3600000,  "LSB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("GYA WEAFAX 1    ", 2616600,  2616600,  2616600,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("GYA WEAFAX 2    ", 4608100,  4608100,  4608100,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("GYA WEAFAX 3    ", 8038100,  8038100,  8038100,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("GYA WEAFAX 4    ", 11084600,  14434100,  14434100,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("DWD WEAFAX 1    ", 3853100,  18259100,  18259100,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("DWD WEAFAX 2    ", 7878100,  18259100,  18259100,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("DWD WEAFAX 3    ", 13880600,  18259100,  18259100,  "USB", "SLOW", "500Hz", "Normal"))
mem.add(channel_to_words("RAF VOLMET      ", 5450000,  5480000,  5730000,  "USB", "SLOW", "10Hz", "Normal"))
mem.add(channel_to_words("SHANNON VOLMET  ", 5505000,  5480000,  5730000,  "USB", "SLOW", "10Hz", "Normal"))

#100 - 200 CB frequencies
cb_start = 27601250
cb_stop = 27991250
cb_step = 10000
for channel in range(40):
    mem.add(channel_to_words("CB UK %2u        "%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", "Normal"))

cb_start = 26965000
cb_stop = 27405000
cb_step = 10000
for channel in range(40):
    mem.add(channel_to_words("CB CEPT %2u      "%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", "Normal"))

cb_start = 29100000
cb_stop = 29200000
cb_step = 10000
mem.add(channel_to_words("10m FM CALLING  ", cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", "Normal"))
for channel in range(40):
    mem.add(channel_to_words("10m FM SIMPLEX%2u"%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", "Normal"))

mem.generate_c_header()
