from math import *
import numpy as np

class HeadLocal(object):

	MinS = 0.0
	MaxS = 5000.0

	DOF_GRIPPER = 0
	DOF_G = 1
	DOF_A = 2
	DOF_B = 3
	DOF_BASKET_R = 4
	DOF_BASKET_L = 5

	def __init__(self, config):
		self.Config = config
		self.Servos = [(self.MaxS + self.MinS) / 2] * 16
		self.Pins = [''] * 41


	def Shutdown(self):
		pass

        def ExecuteCommand(self, command):
            return "OK local"

	def SetServo(self, id, s):
		print "[%d] = %d" % (id, s)
		self.Servos[id] = s
		return 0

	def GetServo(self, id):
		return self.Servos[id]

	def ReadSensors(self):
		if self.Servos[0] > 60:
			s1 = 1
			if self.Servos[0] > 70:
				s2 = 1
			else:
				s2 = 0
		else:
			s1 = 0
			s2 = 0
		return [s1,s2]

	def ReadServos(self):
		return self.Servos
            
        def RedComapss(self):
            return [0,0]

	# distance from servo1 to servo2
	D = 65

	# distance from servo1 to tip of opened gripper
	M = 120

	# distance from servo1 to tip of closed gripper
	M2 = 140

	# distance from servo1 to tip of closed gripper (for SmartA)
	M3 = 157

	# distance from floor to servo2
	H = 177

	# distance from servo1 camera base
	C = 30

	# distance from camera base to camera center
	R = 30
	# return time
	# camera angle correction
	#CC = 0.14486232791552936
	#CC = 0.11963216302759028
	#CC = -0.04330822659124012
	#CC = -0.05
	# atan((CX-c) / (H-CZ)) = atan((2D +R - c) / (H - C))
	# ->123
	#CC = 0.24657866176153292
	# ->130
	CC = 0.201317108375


	# camera horizontal angle of view
	CH = 0.9773843811168246

	# camera vertical angle of view
	CV = 0.5410520681182421

	# safe distance from the floor, i.e. it is max object height
	SH = 35

	# distance from the floor to grab an object
	#GRH = 32
	GRH = 20

	# center of gripper
	GRS = 10

	def SetB(self, b):
		self.SetServo(self.DOF_B, b)
	
	def SetAngle(self, dof, angle):
		self.SetServo(self.DOF_B, (self.MaxS - self.MinS) * (angle + pi) / pi / 2 + self.MinS)

	def IsReleased(self):
		return self.Servos[self.DOF_GRIPPER] == 0

	# return time
	def Release(self):
		return self.SetServo(self.DOF_GRIPPER, 0)
	
	def GrabStatus(self):
		s = self.ReadSensors()
		status = 0
		for sn in s:
			if sn == 0:
				status = status + 1
		return status

	SAFE_A = 1300
	MAX_G = 4745

	def GetDistance(self):
		d_c = [1.99284446e-05, -1.33031509e-01, 3.03207967e+02]
		a = self.GetServo(self.DOF_A)
		#g supposed to be self.MAX_G
		return ((d_c[0] * a) + d_c[1]) * a + d_c[2]

	BASKET_RIGHT = 0
	BASKET_LEFT = 1

	BASKET_POSITIONS = [{"dof": DOF_BASKET_R, "back": 4300, "take": 950, "B": 0},
	                    {"dof": DOF_BASKET_L, "back": 300, "take": 3865, "B": 5000}]

	def BasketBack(self, basketId):
		basket = self.BASKET_POSITIONS[basketId]
		return self.SetServo(basket["dof"], basket["back"])

	def BasketTake0(self):
		time1 = self.SetServo(self.DOF_G, 3570)
		time2 = self.SetServo(self.DOF_A, 0)
		return max(time1, time2)

	def BasketTake(self, basketId):
		basket = self.BASKET_POSITIONS[basketId]
		time1 = self.SetServo(basket["dof"], basket["take"])
		time2 = self.SetServo(self.DOF_B, basket["B"])
		return max(time1, time2)

	def CountPredictionParameters(self, positions, results):
		a = np.array(positions)
		b = np.array(results)
		x = np.linalg.solve(a, b)
		return x.tolist()
            
        def CountPredictionValue(self, parameters, d):
            return (parameters[2] * d + parameters[1]) * d + parameters[0]

	def SetLED(self, value):
		pass
