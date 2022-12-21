import numpy as np
import random
import copy
import os

image_set = [None, None, None, None, None, None, None]

with open("imageBib/left1", "rb") as imageFile:
    image_set[0] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/left23", "rb") as imageFile:
    image_set[1] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/left64", "rb") as imageFile:
    image_set[2] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/vertical", "rb") as imageFile:
    image_set[3] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/right1", "rb") as imageFile:
    image_set[4] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/right23", "rb") as imageFile:
    image_set[5] = np.fromfile(imageFile, dtype=np.uint8)

with open("imageBib/right64", "rb") as imageFile:
    image_set[6] = np.fromfile(imageFile, dtype=np.uint8)

# make sure the images directory exists 
if not os.path.exists("images"):
    os.makedirs("images")

object_x = 45
object_width = 20
object_height = 20
direction_choice = 0
if len([x for x in image_set if x is not None]) == len(image_set):
    for i in range(1, 100):
        direction_choice = random.randint(3, 6)
        with open("images/img" + str(i) + ".raw", "wb") as binary_file:
            selected_image = copy.deepcopy(
                image_set[direction_choice].reshape(128, 128))
            object_y = i + 20
            if object_y > 0 and object_y < 128:
                selected_image[i:i+object_height,
                               object_x:object_x+object_width] = 128
            # if direction_choice < 2:
            #    object_x += 1 + direction_choice
            if direction_choice > 2:
                if object_x > 0:
                    object_x -= 1
                elif object_width > 0:
                    object_width -= 1
            binary_file.write(bytes(bytearray(selected_image)))
    # Generate image
