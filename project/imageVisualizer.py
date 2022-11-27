from PIL import Image
import io
with open("rawImage", "rb") as imageFile:
    f = imageFile.read()
    image = Image.frombytes('L', (16,16), bytes(f))
    image.show()