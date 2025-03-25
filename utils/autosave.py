#!/usr/bin/env python
import struct

channel_size = 32
num_channels = 512
class Memory:
  def __init__(self):
    self.memory = []

  def generate_c_header(self):
      while len(self.memory) < num_channels:
        self.memory.append([0xffffffff for i in range(channel_size)])
      data = ",\n".join(["{"+", ".join([str(i) for i in data])+"}" for data in self.memory])
      buffer = """
#include <pico.h>
#include "autosave_memory.h"
const uint32_t __in_flash() __attribute__((aligned(4096))) autosave_memory[%s][%s] = {
%s
};"""%(num_channels, channel_size, data)
      with open("autosave_memory.cpp", "w") as outf:
        outf.write(buffer)

mem = Memory()
mem.generate_c_header()
