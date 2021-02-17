#!/usr/bin/env python3
#
# Replace an image with an empty placeholder of the same size.
#

import sys
from PIL import Image

for img in sys.argv[1:]:
    print('Processing', img)
    orig = Image.open(img)
    width, height = orig.size
    new = Image.new("RGBA", (width, height), (255, 255, 255, 0))
    new.save(img, orig.format)
