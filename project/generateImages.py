import numpy as np
import random

image_set = [None, None, None]

with open("imageBib/left1", "rb") as imageFile:
    image_set[0] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/vertical", "rb") as imageFile:
    image_set[1] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/right1", "rb") as imageFile:
    image_set[2] = np.fromfile(imageFile, dtype=np.uint8)
if len([x for x in image_set if x is not None]) == len(image_set):
    for i in range(1,100):
        with open("images/img" + str(i) + ".raw", "wb") as binary_file:
            binary_file.write(bytes(bytearray(image_set[random.randint(0,2)])))
    # Generate image
    