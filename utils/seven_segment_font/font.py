import cairo
import numpy as np
from matplotlib import pyplot as plt
import imageio
import sys

def read_character(width, height, character, font, of):

  #read image and convert to C header file
  im = imageio.imread(str(ord(character))+".png")
  h, w, c = im.shape
  assert w == width
  assert h == height

  glyph = []
  bits_done = 0
  data = 0
  for x in range(w):
    for y in range(h):
      r, g, b = im[y][x][:3]
      data >>= 1
      if r == 0:
        data |= 128
      bits_done += 1
      if bits_done == 8:
        glyph.append(data)
        data = 0
        bits_done = 0
  data >>= 8-bits_done
  glyph.append(data)

  of.write(" ".join(["0x%02x,"%i for i in glyph])+"//%s\n"%character.replace("\\", "backslash"))

def generate_font(height, width, first_char, last_char, font):
  name = "font_%s_%ux%u"%(font, height, width)
  with open(name+".h", "w") as of:

    of.write(
"""#ifndef __%s__
#define __%s__

/*
 * Format
 * <height>, <width>, <additional spacing per char>, 
 * <first ascii char>, <last ascii char>,
 * <data>
 */
const uint8_t %s[] =
{
"""%(name, name, name))
    of.write("%u, %u, %u, %u, %u,\n"%(height, width, 1, first_char, last_char))
    characters = "".join([chr(i) for i in range(first_char, last_char+1)])
    for character in characters:
      read_character(width, height, character, font, of)
    of.write(
"""};
#endif
""")

generate_font(25, 15, ord("0"), ord("9")+1, "seven_segment")

