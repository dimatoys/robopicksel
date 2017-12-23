import time
import sys

sys.path.append('.')

def test1():
	from Head import RangeVL53LOX

	range = RangeVL53LOX()
	print range.GetDistance()
	range.Shutdown()

def test2():
	from Head import HMC5883
	compass = HMC5883(1, 0, 0, 0)
	while True:
		try:
			print compass.getXYZ()
			time.sleep(100/1000000.00)
		except:
			print "Stop"
			break

def test3():
	from Head import HMC5883, RangeVL53LOX
	compass = HMC5883(1, 0, 0, 0)
	range = RangeVL53LOX()
	while True:
		try:
			print compass.getXYZ(), range.GetDistance()
			time.sleep(100/1000000.00)
		except:
			print "Stop"
			break
	range.Shutdown()

#test1()
#test2()
test3()
