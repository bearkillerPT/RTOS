from PIL import Image
import io
with open("right1", "rb") as imageFile:
    f = imageFile.read()
    image = Image.frombytes('L', (128,128), bytes(f))
    image.show()