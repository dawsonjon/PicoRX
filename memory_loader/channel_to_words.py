#!/usr/bin/env python

#struct s_channel_settings
#{
  #uint32_t frequency;
  #uint32_t max_frequency;
  #uint32_t min_frequency;
  #uint8_t  mode;
  #uint8_t  agc_setting;
  #uint8_t  agc_gain;
  #uint8_t  step;
  #uint8_t  bandwidth;
#};

#struct s_memory_channel
#{
  #s_channel_settings channel;
  #char label[16];
#};

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
bandwidths = {
  "Very Narrow" :0,
  "Narrow":1,
  "Normal":2,
  "Wide":3,
  "Very Wide":4
}

def pack(string):
    return (ord(string[0]) << 0) + (ord(string[1]) << 8) + (ord(string[2]) << 16) + (ord(string[3]) << 24)

def channel_to_words(name, frequency, min_frequency, max_frequency, mode, agc_speed, step, bandwidth):
    if len(name) < 16:
      name += (" "*(16-len(name)))

    #split frequency into bytes
    data = [
      int(frequency)&0xffffffff,
      int(max_frequency)&0xffffffff,
      int(min_frequency)&0xffffffff,
      (modes[mode] << 0) | (agc_speeds[agc_speed] << 8) | (10 << 16) | (steps[step] << 24),
      bandwidths[bandwidth],
      pack(name[0:4]),
      pack(name[4:8]),
      pack(name[8:12]),
      pack(name[12:16]),
      0xffffffff,
      0xffffffff,
      0xffffffff,
      0xffffffff,
      0xffffffff,
      0xffffffff,
      0xffffffff,
    ]

    return data
