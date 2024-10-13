#!/usr/bin/env python
import struct

#define idx_frequency 0
#define idx_mode 1
#define idx_agc_speed 2
#define idx_step 3
#define idx_max_frequency 4
#define idx_min_frequency 5
#define idx_squelch 6
#define idx_volume 7
#define idx_cw_sidetone 8
#define idx_hw_setup 9
#define idx_gain_cal 10
#define idx_bandwidth 11
#define idx_rx_features 12
#define idx_oled_contrast 13  // values 0..15 get * 17 for set_contrast(0.255)



modes = {
  "AM" :0,
  "AMS" :1,
  "LSB":2,
  "USB":3,
  "NFM":4,
  "CW" :5,
  255:0xffffffff
}
agc_speeds = {"FAST": 0, "NORMAL": 1, "SLOW": 2, "VERY SLOW": 3, 255:0xfffffff}
filter_bandwidths = {"VERY_SLOW": 0, "SLOW": 1, "NORMAL": 2, "FAST": 3, "VERY_FAST": 4, 255:0xfffffff}
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
  255:0xffffffff
}


num_channels = 512
channel_size = 16
class Memory:
  def __init__(self):
    self.memory = []

  def add(self, frequency, min_frequency, max_frequency, mode, agc_speed, step, cw_sidetone, volume, squelch, gain_cal, filter_bandwidth):

      #split frequency into bytes
      data = [
        int(frequency)&0xffffffff,           #0
        modes[mode],                         #1
        agc_speeds[agc_speed],               #2
        steps[step],                         #3
        int(max_frequency)&0xffffffff,       #4
        int(min_frequency)&0xffffffff,       #5
        squelch,                             #6
        volume,                              #7
        int(cw_sidetone),                    #8 (100Hz steps)
        0x00000780,                          #9	hw_setup
        int(gain_cal),                       #a
        0x10 | filter_bandwidths[filter_bandwidth], #b zoom==1
        0x00000000,                          #c rx_features
        0x10080402,                          #d band limits 1 (125kHz steps)
        0x00804020,                          #e band limits 2 (125kHz steps)
        0xffffffff,                          #f
      ]
      self.memory.append(data)

  def generate_c_header(self):
      while len(self.memory) < num_channels:
        self.memory.append([0xffffffff for i in range(channel_size)])
      data = ",\n".join(["{"+", ".join([str(i) for i in data])+"}" for data in self.memory])
      buffer = """#ifndef __autosave_memory__
#define __autosave_memory__

#include <pico/platform.h>
const uint32_t __in_flash() __attribute__((aligned(4096))) autosave_memory[%s][%s] = {
%s
};
#endif"""%(num_channels, channel_size, data)
      with open("autosave_memory.h", "w") as outf:
        outf.write(buffer)

mem = Memory()
mem.add(1413000,  0,   30e6,  "AM", "VERY SLOW", "1kHz", 10, 5, 0, 62, "NORMAL")
mem.generate_c_header()
