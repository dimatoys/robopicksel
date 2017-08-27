import sys
sys.path.append('..')

from math import *

from Geometry import Geometry
from HeadLocal import HeadLocal

'''
3500: (58 - 40) = 18 (max) 8 (+10)
{"rby": 22.0, "rbx": 8.0, "lby": -16.0, "lbx": 8.0, "lty": -23.0, "ltx": -82.0, "y": 4.0, "x": -28.0, "rty": 33.0, "rtx": -82.0}

3250: (92 - 40) = 52 (max) 42 (+10)
{"rby": 20.0, "rbx": 42.0, "lby": -16.0, "lbx": 42.0, "lty": -21.0, "ltx": -25.0, "y": 3.0, "x": 14.0, "rty": 28.0, "rtx": -25.0}

3000: (119 - 40) = 79 (max) 71 (+8)
{"rby": 16.0, "rbx": 71.0, "lby": -21.0, "lbx": 71.0, "lty": -26.0, "ltx": 17.0, "y": -3.0, "x": 47.0, "rty": 20.0, "rtx": 17.0}

2750: (144 - 40) = 104 (max) 97 (+7)
{"rby": 21.0, "rbx": 97.0, "lby": -16.0, "lbx": 97.0, "lty": -19.0, "ltx": 49.0, "y": 2.0, "x": 75.0, "rty": 24.0, "rtx": 49.0}

2500: (170 - 40) = 130 (max) 113 (+17)
{"rby": 15.0, "rbx": 113.0, "lby": -22.0, "lbx": 113.0, "lty": -25.0, "ltx": 67.0, "y": -3.0, "x": 92.0, "rty": 18.0, "rtx": 67.0}

2250: (196 - 40) = 156 (min) ?
{"rby": 25.0, "rbx": 123.0, "lby": -12.0, "lbx": 123.0, "lty": -13.0, "ltx": 77.0, "y": 7.0, "x": 101.0, "rty": 28.0, "rtx": 77.0}

2000: (217 - 40) = 177 (min) 176 (+1)
{"rby": 11.0, "rbx": 215.0, "lby": -30.0, "lbx": 215.0, "lty": -29.0, "ltx": 176.0, "y": -9.0, "x": 195.0, "rty": 10.0, "rtx": 176.0}

1500: (279 - 40) = 239 (min) 243 (-4)
{"rby": 18.0, "rbx": 290.0, "lby": -25.0, "lbx": 290.0, "lty": -23.0, "ltx": 243.0, "y": -3.0, "x": 265.0, "rty": 17.0, "rtx": 243.0}

1000: (360 - 40) = 320 (min) 342 (-22)
{"rby": 24.0, "rbx": 404.0, "lby": -22.0, "lbx": 404.0, "lty": -19.0, "ltx": 342.0, "y": 1.0, "x": 371.0, "rty": 21.0, "rtx": 342.0}
'''

def MainFunction(a):
	global Head
	global geometry
	head.SetServo(geometry.DOF_A, a)
	locator = geometry.GetLocator()
	(x,y) = locator.GetXYFromAngles(0,0)
	return x

def FindValue(a, value):
	for  i in range(2000):
		f = MainFunction(a)
		if f == value:
			return a
		if f < value:
			a -= 1
		else:
			a += 1
	return None

head = HeadLocal()
geometry = Geometry(head)

#print geometry.CountCC(130)

head.SetServo(geometry.DOF_G, head.MaxS)
head.SetServo(geometry.DOF_B, (head.MinS + head.MaxS) / 2)

values = [(3500, 18), (3250, 52), (3000, 79), (2750, 104), (2500, 130), (2250, 156), (2000, 177), (1500, 239), (1000, 320)]

result = {}

for (a,v) in values:
	result[a] = FindValue(a,v)

for (a,v) in values:
	print "%d: %d init error=%d" % (a, result[a], MainFunction(a) - v)

