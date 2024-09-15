import cairo
import numpy as np
from matplotlib import pyplot as plt
import imageio
import sys

def draw_character(width, height, x, y, character, font, of):

  #create an image of each glyph using cairo
  surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
  ctx = cairo.Context(surface)
  ctx.scale(1, 1)
  ctx.set_antialias(cairo.ANTIALIAS_NONE)

  ctx.rectangle(0, 0, width, height)
  ctx.set_source_rgba(1, 1, 1)
  ctx.fill()

  ctx.set_source_rgba(0, 0, 0)
  ctx.set_font_size(height)
  matrix = cairo.Matrix(xx=width, yy=height)
  ctx.set_font_matrix(matrix)
  ctx.select_font_face(font,
                     cairo.FONT_SLANT_NORMAL,
                     cairo.FONT_WEIGHT_NORMAL)
  fo = cairo.FontOptions()
  fo.set_antialias(cairo.ANTIALIAS_NONE)
  ctx.set_font_options(fo)
  
  ctx.move_to(x, y)
  ctx.show_text(character)
  surface.write_to_png(str(ord(character))+".png")

  #read image and convert to C header file
  im = imageio.imread(str(ord(character))+".png")
  h, w, c = im.shape

  glyph = []
  for x in range(w-1):
    bits_done = 0
    data = 0
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

  of.write(" ".join(["0x%02x,"%i for i in glyph])+"//%s\n"%character.replace("\\", "backslash"))

def generate_font(height, width, x, y, first_char, last_char, font):
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
    of.write("%u, %u, %u, %u, %u,\n"%(height, width-1, 1, first_char, last_char))
    characters = "".join([chr(i) for i in range(first_char, last_char+1)])
    for character in characters:
      draw_character(width, height, x, y, character, font, of)
    of.write(
"""};
#endif
""")

#generate_font(16, 12, 1, 13, 32, 127, "Chilanka")
#generate_font(16, 12, 1, 13, 32, 127, "Dhurjati")
generate_font(16, 12, 1, 13, 32, 127, "Liberation-Mono")
#generate_font(8, 6, 1, 6, 32, 127, "Ubuntu")

