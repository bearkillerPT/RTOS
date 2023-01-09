from PIL import Image
import imageio

images = []
for i in range(1,100):
    with open("images/img" + str(i) + ".raw", "rb") as imageFile:
        images.append(Image.frombytes('L', (128,128), bytes(imageFile.read())))
imageio.mimsave('imagesGif.gif', images)


#with open("imageBib/vertical", "rb") as imageFile:
#    f = imageFile.read()
#    image = Image.frombytes('L', (128,128), bytes(f))
#    image.show()