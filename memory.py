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
  "LSB":1,
  "USB":2,
  "NFM":3,
  "WFM":4,
  "CW" :5
}
agc_speeds = {"FAST": 0, "NORMAL": 1, "SLOW": 2, "VERY SLOW": 3}
steps = {
    "10Hz": 0,
    "50Hz": 1,
    "100Hz": 2,
    "1kHz": 3,
    "5kHz": 4,
    "10kHz": 5,
    "12.5kHz": 6,
    "25kHz": 7,
    "50kHz": 8,
    "100kHz": 9,
}

def pack(string):
    return (ord(string[0]) << 24) + (ord(string[1]) << 16) + (ord(string[2]) << 8) + ord(string[3])

def program(page, name, frequency, min_frequency, max_frequency, mode, agc_speed, step, squelch):
    assert len(name) == 16

    #split frequency into bytes
    data = [
      int(frequency)&0xffffffff,
      modes[mode],
      agc_speeds[agc_speed],
      steps[step],
      int(max_frequency)&0xffffffff,
      int(min_frequency)&0xffffffff,
      pack(name[0:4]),
      pack(name[4:8]),
      pack(name[8:12]),
      pack(name[12:16]),
    ]

    print("{", ", ".join([str(i) for i in data]), "},")

pchan = iter(range(1, 499))
program(next(pchan), "MW Broadcast    ", 1215000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "LW Broadcast    ", 198000,   153000,   279000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW        120m  ", 2300000,  2300000,  2495000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         90m  ", 3200000,  3200000,  3400000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         75m  ", 3900000,  3900000,  4000000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         60m  ", 4750000,  4750000,  4995000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         49m  ", 5900000,  5900000,  6200000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         41m  ", 7200000,  7200000,  7450000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         31m  ", 9400000,  9400000,  9900000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         25m  ", 11600000, 11600000, 12100000, "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         22m  ", 13570000, 13570000, 13870000, "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         19m  ", 15100000, 15100000, 15800000, "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         16m  ", 17480000, 17480000, 17900000, "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         15m  ", 18900000, 18900000, 19020000, "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         13m  ", 21450000, 21450000, 21850000, "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SW         11m  ", 25670000, 25670000, 26100000, "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "10m SSB         ", 28300000, 28300000, 29000000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "12m SSB         ", 24931000, 24931000, 24990000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "15m SSB         ", 21151000, 21151000, 21450000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "17m SSB         ", 18111000, 18111000, 18168000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "20m SSB         ", 14101000, 14101000, 14350000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "40m SSB         ", 7060000,  7060000,  7200000,  "LSB", "SLOW", "100Hz", 0)
program(next(pchan), "80m SSB         ", 3620000,  3620000,  3800000,  "LSB", "SLOW", "100Hz", 0)
program(next(pchan), "160m SSB        ", 1843000,  1843000,  2000000,  "LSB", "SLOW", "100Hz", 0)
program(next(pchan), "10m CW          ", 28000000, 28000000, 28070000, "CW", "SLOW", "10Hz", 0)
program(next(pchan), "12m CW          ", 24890000, 24890000, 24915000, "CW", "SLOW", "10Hz", 0)
program(next(pchan), "15m CW          ", 21000000, 21000000, 21070000, "CW", "SLOW", "10Hz", 0)
program(next(pchan), "17m CW          ", 18068000, 18068000, 18095000, "CW", "SLOW", "10Hz", 0)
program(next(pchan), "20m CW          ", 14000000, 14000000, 14099000, "CW", "SLOW", "10Hz", 0)
program(next(pchan), "30m CW          ", 10100000, 10100000, 10130000, "CW", "SLOW", "10Hz", 0)
program(next(pchan), "40m CW          ", 7000000,  7000000,  7060000,  "CW", "SLOW", "10Hz", 0)
program(next(pchan), "80m CW          ", 3500000,  3500000,  3620000,  "CW", "SLOW", "10Hz", 0)
program(next(pchan), "160m CW         ", 1810000,  1810000,  1843000,  "CW", "SLOW", "10Hz", 0)
program(next(pchan), "10m PSK         ", 28120000, 28070000, 28190000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "12m PSK         ", 24920000, 24195000, 24929000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "15m PSK         ", 21080000, 21070000, 21149000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "17m PSK         ", 18097000, 18095000, 18109000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "20m PSK         ", 14070000, 14070000, 14099000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "30m PSK         ", 10141000, 10130000, 10150000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "40m PSK         ", 7040000,  7000000,  7060000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "80m PSK         ", 3580000,  3500000,  3620000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "160m PSK        ", 1830000,  1810000,  1843000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "10m FT8         ", 28074000, 28070000, 28190000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "12m FT8         ", 24915000, 24195000, 24929000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "15m FT8         ", 21074000, 21070000, 21149000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "17m FT8         ", 18100000, 18095000, 18109000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "20m FT8         ", 14074000, 14070000, 14099000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "30m FT8         ", 10136000, 10130000, 10150000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "40m FT8         ", 7074000,  7040000,  7060000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "80m FT8         ", 3573000,  3570000,  3600000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "160m FT8        ", 1840000,  1810000,  1843000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "10m RTTY        ", 28080000, 28070000, 28190000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "12m RTTY        ", 24925000, 24195000, 24929000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "15m RTTY        ", 21080000, 21070000, 21149000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "17m RTTY        ", 18106000, 18095000, 18109000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "20m RTTY        ", 14083000, 14070000, 14099000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "30m RTTY        ", 10143000, 10130000, 10150000, "USB", "SLOW", "100Hz", 0)
program(next(pchan), "40m RTTY        ", 7043000,  7040000,  7060000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "80m RTTY        ", 3590000,  3570000,  3600000,  "USB", "SLOW", "100Hz", 0)
program(next(pchan), "BBC RADIO 5 Live", 693000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "BBC Her and worc", 738000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "855 Sunshine    ", 855000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "Talk Sport      ",1053000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "BBC Gloucester  ",1215000,  531000,   1602000,  "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "BBC Radio 4     ", 198000,   153000,   279000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "RTE Radio 1     ", 252000,   153000,   279000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "SHANNON VOLMET  ", 5505000,  5480000,  5730000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON VOLMET 2", 5662000,  5480000,  5730000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON ATC    1", 5598000,  5480000,  5730000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON ATC    2", 5616000,  5480000,  5730000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON ATC    3", 5649000,  5480000,  5730000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON ATC    4", 5658000,  5480000,  5730000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON ATC    5", 8906000,  8815000,  9040000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON ATC    6", 8864000,  8815000,  9040000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "SHANNON ATC    7", 8879000,  8815000,  9040000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "KINLOSS RESCUE1 ", 5680000,  5598000,  5505000,  "USB", "SLOW", "10Hz", 0)
program(next(pchan), "BBC Word Serv 1 ", 3915000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "BBC Word Serv 2 ", 5890000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "BBC Word Serv 3 ", 12095000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "BBC Word Serv 4 ", 15335000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 1   ", 909000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 2   ", 1530000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 3   ", 4930000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 4   ", 4960000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 5   ", 6080000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 6   ", 9550000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 7   ", 13590000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 8   ", 15580000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "V O America 9   ", 17895000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "RADIO JAPAN 1   ", 5955000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "RADIO JAPAN 2   ", 6104000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "RADIO JAPAN 3   ", 11860000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "RADIO JAPAN 4   ", 11935000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 1", 5960000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 2", 7285000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 3", 7350000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 4", 7415000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 5", 9470000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 6", 9600000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 7", 9675000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 8", 11940000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INTL 9", 11965000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 10", 12015000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 11", 13665000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 12", 13670000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 13", 13760000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 14", 13710000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 15", 15245000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 16", 17490000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 17", 17570000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 18", 17630000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "CHINA RAD INT 19", 17650000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "UNITES NATIONS 1", 9565000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "UNITES NATIONS 2", 17810000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 1       ", 6185000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 2       ", 7230000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 3       ", 7235000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 4       ", 7250000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 5       ", 7360000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 6       ", 9645000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 7       ", 9705000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 8       ", 11620000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 9       ", 11815000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 10      ", 15595000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)
program(next(pchan), "VATICAN 11      ", 17790000,   153000,   30000000,   "AM", "VERY SLOW", "1kHz", 0)



#100 - 200 CB frequencies
cb_start = 27601250
cb_stop = 27991250
cb_step = 10000
for channel in range(40):
    program(next(pchan),"CB UK %2u        "%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", 0)

cb_start = 26965000
cb_stop = 27405000
cb_step = 10000
for channel in range(40):
    program(next(pchan),"CB CEPT %2u      "%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", 0)

cb_start = 29100000
cb_stop = 29200000
cb_step = 10000
program(next(pchan),"10m FM CALLING  ", cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", 0)
for channel in range(40):
    program(next(pchan),"10m FM SIMPLEX%2u"%(channel+1), cb_start+(cb_step*channel),  cb_start,  cb_stop,  "NFM", "FAST", "5kHz", 0)
