#! /usr/bin/env python3

import sys
import cv2 as cv
import numpy as np

img = cv.imread(sys.argv[1])
index = {(*val,): i for i, val in enumerate(img[0][:249])}

img = img.reshape((320 * 200, 3))
binimg = bytes([index[(*x,)] for x in img])

with open(sys.argv[2], 'wb') as fout:
    fout.write(binimg)
