from PIL import Image
# import imageio
import numpy as np


def fullprint(*args, **kwargs):
    from pprint import pprint
    import numpy
    opt = numpy.get_printoptions()
    numpy.set_printoptions(threshold=numpy.inf)
    pprint(*args, **kwargs)
    numpy.set_printoptions(**opt)


images = []
# for i in range(1,100):
#     with open("images/img" + str(i) + ".raw", "rb") as imageFile:
#         images.append(Image.frombytes('L', (128,128), bytes(imageFile.read())))
# imageio.mimsave('imagesGif.gif', images)


with open("imageBib/left23", "rb") as imageFile:
    f = imageFile.read()
    image = Image.frombytes('L', (128, 128), bytes(f))
    image_bytes = list(bytearray(f))
    fullprint(np.array(image_bytes).reshape(128, 128))

    #    image.show()
