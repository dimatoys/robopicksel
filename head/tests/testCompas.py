import sys
sys.path.append('.')

from Head import HMC5883

compas0 = HMC5883(0)
compas1 = HMC5883(1)

while True:
    print compas1.getAngle(), compas0.getAngle()


