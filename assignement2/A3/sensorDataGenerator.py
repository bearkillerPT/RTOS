import numpy as np

s = [int(x) for x in np.random.normal(65, 5, 1000000)]
data = np.array(s).astype(np.uint16)
data.tofile(open('sensorData.txt', 'w'))