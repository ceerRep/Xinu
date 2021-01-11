#! /usr/bin/env python3

import sys
import cv2 as cv
import numpy as np

img_bgr = cv.imread(sys.argv[1])
img_rgb = cv.cvtColor(img_bgr, cv.COLOR_BGR2RGB)
img = img_rgb.reshape((320*200, 3))
colors = np.unique(img, axis=0)
colors_array = [*colors.flatten()]
index = {(*val,): i for i, val in enumerate(colors)}

assert len(index) == len(colors)

with open(sys.argv[2], 'wb') as fout:
    fout.write(bytes(colors_array))
    fout.write(b'\0\0\0' * (256 - len(colors)))
    fout.write(bytes([index[(*x,)] for x in img]))
