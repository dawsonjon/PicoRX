import imageio.v2
import sys

input_file = sys.argv[1]
name = input_file.split(".")[0]
output_file = name+".h"

im = imageio.v2.imread(input_file)
h, w, c = im.shape
print(w, h)

pixels = []
for y in range(h):
  for x in range(w):
    r, g, b = im[y][x][:3]
    r = int(r)
    g = int(g)
    b = int(b)
    color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | b >> 3
    color = ((color & 0xff00) >> 8 | (color & 0xff) << 8)
    pixels.append("0x%04x"%color)
pixels = ",\n".join(pixels)
contents = "static const uint16_t __in_flash() %s[] = {%s};"% (name, pixels)

with open(output_file, "w") as outf:
  outf.write(contents)
