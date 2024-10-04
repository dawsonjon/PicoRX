import argparse
from pathlib import Path

import numpy as np
from PIL import Image

DISPLAY_WIDTH = 128
DISPLAY_HEIGHT = 64
OUT_IMG_SCALE = 4
ON_PIXEL_COLOR = (255, 255, 255)

parser = argparse.ArgumentParser(
    description="Tool for decoding screen dump log to PNGs."
)

parser.add_argument("log_path", help="Path to the screen dump log file")

args = parser.parse_args()

input_file = Path(args.log_path)

out_dir = Path("img")
out_dir.mkdir(exist_ok=True)

with open(input_file, "r") as f:
    c = 0
    fc = 0

    img_arr = []
    started = False

    for l in f.readlines():
        l = l.strip()
        t = l.split(",")[:-1]

        if started and len(t) == 0:
            img_arr = np.array(img_arr)
            img = Image.new(
                "RGB", (DISPLAY_WIDTH * OUT_IMG_SCALE, DISPLAY_HEIGHT * OUT_IMG_SCALE)
            )
            for y in range(DISPLAY_HEIGHT):
                for x in range(DISPLAY_WIDTH):
                    if img_arr[y // 8][x] & (1 << (y & 7)):
                        for i in range(OUT_IMG_SCALE):
                            for j in range(OUT_IMG_SCALE):
                                img.putpixel(
                                    (OUT_IMG_SCALE * x + i, OUT_IMG_SCALE * y + j),
                                    ON_PIXEL_COLOR,
                                )
            img_fn = f"img/{input_file.stem}_{fc:06}.png"
            print(f"Saving image {img_fn}")
            img.save(img_fn)
            img_arr = []
            started = False
            fc += 1
        elif len(t) == 0:
            started = True
        elif started:
            img_arr.append(list(map(lambda x: int(x, 16), t)))

print("To convert PNGs to GIF (on Linux) use the following command:")
print(
    f"convert -delay 10 {input_file.stem}*.png -layers Optimize {input_file.stem}.gif"
)
