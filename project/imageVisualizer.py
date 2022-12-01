from PIL import Image
import io
with open("imageBib/vertical", "rb") as imageFile:
    f = imageFile.read()
    image = Image.frombytes('L', (128,128), bytes(f))
    image.show()