import threading
import time
import datetime
import sys
from math import *

from Camera import VisionInstance, ExtractorSet, ExtractorGet, ExtractorParmeters, CameraGetValues, CameraSetValues, LearnExtractor, DeleteDump
from Geometry import TLearning

class Commands(threading.Thread):

	SUCCESS = "ok"
	FAIL = "fail"

	MAX_LOOK_ATTEMPTS = 10
	MAX_PICK_ATTEMPTS = 4
	MAX_LIFT_ATTEMPTS = 4
	MOVE_ATTEMPTS = 5

	GRAB_D_MIN = 155
	GRAB_D_MAX = 237
	MOVE_D_TARGET = 150
	D_TO_MOVE = 200.0

	def __init__(self, head, logger, config=None):
		threading.Thread.__init__(self)
		self.config = config
		self.daemon = True
		self.Head = head
		self.ExitFlag = False
		self.Logger = logger
		self.Cmd = None
		self.LastStatus = None
		self.LastResult = None
		self.Lock = threading.Condition()
		self.VarCamera = None

		if config is not None:
			self.Learning = TLearning(config)
			self.Learning.LearnGrabPositions()
			self.Learning.LearnCameraY()
			self.Views = {3000: {"D":  34.0, "H": 139.0, "W1": 185.0, "W2": 150.0},
			              2500: {"D": 104.0, "H": 132.0, "W1": 174.0, "W2": 180.0},
			              1500: {"D": 224.0, "H": 235.0, "W1": 198.0, "W2": 320.0}}
			self.DMIN = 130
			self.DMAX = 230
			self.WMIN = 5
			self.WMAX = 55



		self.ControlPoints = { "OPEN": {"FARTHEST": {"g": 1261, "a": 3415, "d": 219.85},
                                                "CLOSEST":  {"g": 3831, "a": 2491, "d": 106},
		                                "MID":      {"g": 3383, "a": 2202, "d": 164}},
		                       "CLOSE":{"FARTHEST": {"g": 3431, "a": 1537, "d": 219.85},
                                                 "CLOSEST": {"g": 4793, "a": 1226, "d": 106},
		                                 "MID":     {"g": 4300, "a": 1152, "d": 164}},
		                       "R":    {"FARTHEST": {"a": 0,    "r": 237},
                                                 "CLOSEST": {"a": 2638, "r": 70},
		                                 "MID":     {"a": 1013, "r": 135}}}

		self.ControlPrediction = {"OPEN":  {"g": [27.692678537972824, 64.06358666520083, -0.2658804614675382],
		                                    "a": [7096.304232843295, -68.3068262198355, 0.23453358370054014]},
		                          "CLOSE": {"g": [4616.0645833996805, 8.241978973888987, -0.06200732953292217],
		                                    "a": [2608.633346493787, -20.64975939636596, 0.07175517528666829]},
                                          "R":     {"r":[]},
                                          "D":     {"d":[3.03207967e+02, -1.33031509e-01, 1.99284446e-05]}}

                self.FIND_POSITIONS = [{self.Head.DOF_A: 1764, self.Head.DOF_B: 2500},
		              {self.Head.DOF_A: 700, self.Head.DOF_B: 2500},
		              {self.Head.DOF_A: 1764, self.Head.DOF_B: 3600},
		              {self.Head.DOF_A: 1764, self.Head.DOF_B: 4600},
		              {self.Head.DOF_A: 1764, self.Head.DOF_B: 1400},
		              {self.Head.DOF_A: 1764, self.Head.DOF_B: 400},
		              {self.Head.DOF_A: 700, self.Head.DOF_B: 1500},
		              {self.Head.DOF_A: 700, self.Head.DOF_B: 3500},
		              {self.Head.DOF_A: 0, self.Head.DOF_B: 3600},
		              {self.Head.DOF_A: 0, self.Head.DOF_B: 2500},
		              {self.Head.DOF_A: 0, self.Head.DOF_B: 1400}]

                self.IN_REACH_DISTANCE_POSITIONS = 8

	def ShutdownCamera(self): 
		if self.VarCamera is not None:
			#self.CmdPrint("run: Release camera")
			self.VarCamera.Release()
			#self.CmdPrint("run: Ok Release camera")
			self.VarCamera = None

	def run(self):
		self.Lock.acquire()
		while True:
			while self.Cmd is None:
				self.Lock.wait()
				if self.ExitFlag:
					self.Lock.release()
					return
			self.LastResult = None
			try:
				self.CmdPrint("Cmd: %s" % (self.Cmd))
				self.LastStatus = eval("self.%s" % self.Cmd)
			except:
				import traceback
				T, V, TB = sys.exc_info()
				self.CmdPrint("Unexpected error: %s" % ''.join(traceback.format_exception(T,V,TB)))
				self.LastStatus = self.FAIL
				self.LastResult = "Exception"
			self.ShutdownCamera()
			self.Cmd = None


	def Start(self, strCmd):
		if self.Cmd is None:
			self.Cmd = strCmd
			self.Lock.acquire()
			self.Lock.notify()
			self.Lock.release()
			return True
		else:
			return False
	
	def Shutdown(self):
		self.ExitFlag = True
		self.Lock.acquire()
		self.Lock.notify()
		self.Lock.release()

	def Sleep(self, sec):
		time.sleep(sec)

	def SetResult(self, result):
		self.LastResult = result

	def InitCameraNonBlocking(self):
		#self.CmdPrint("Init camera non blocking test:" + str(self.VarCamera) + " from:" + str(self))
		if self.VarCamera is None:
			self.CmdPrint("Init camera non blocking")
			self.VarCamera = VisionInstance()
		#self.CmdPrint("Init camera non blocking test Ok:" + str(self.VarCamera))

	def CmdExtractorGetParameters(self):
		parameters = ExtractorGet()
		parameters.update(CameraGetValues())
		self.CmdPrint(str(parameters))
		self.SetResult(parameters)
		
		return self.SUCCESS

	def CmdExtractorSetParameters(self, values):
		#for param, value in values.items():
		#	self.CmdPrint("CmdExtractorSetParameters: %s = %s" % (param, value))
		ExtractorSet(values)
		CameraSetValues(values)
		return self.SUCCESS

	def CmdCameraFire(self, file=None):
		self.CmdPrint("Fire")
		if self.InitCameraNonBlocking() == self.FAIL:
			self.CmdPrint("Init fail")
			return self.FAIL
		if file:
			self.VarCamera.FileName = "static/%s" % file
		else:
			self.VarCamera.FileName = None
		self.VarCamera.Fire()
		if self.VarCamera.IsReady():
			result = {"totalTime": (self.VarCamera.FireEndTime - self.VarCamera.FireStartTime).total_seconds(),
					  "DumpId": self.VarCamera.DumpId}
			if self.VarCamera.FireStartWriteTime:
				result["sootingTime"] = (self.VarCamera.FireStartWriteTime - self.VarCamera.FireStartTime).total_seconds()
			self.SetResult(result)
			return self.SUCCESS
		else:
			self.CmdPrint("Fire fail")
			return self.FAIL

	def CmdPrint(self, message):
		if self.Logger:
			self.Logger.debug(message)
		else:
			print message
		return self.SUCCESS

	def CmdCommand(self, cmd):
		cmd = cmd.strip()
		startTime = datetime.datetime.now()
		result = self.Head.ExecuteCommand(cmd)
		endTime = datetime.datetime.now()
		self.SetResult({"result": result, "time": str(endTime - startTime)})
		return self.SUCCESS

	def CmdOpen(self):
		time = self.Head.Release()
		self.Sleep(time)
		return self.SUCCESS

	def CmdLED(self, value):
		self.Head.SetLED(value)
		return self.SUCCESS

	def SetLookPosition(self, n):
		if self.config.has_option("POSITIONS", "camera.find.%d.A" % n):
			self.CmdPrint("Has option: camera.find.%d.A" %n)
			a = self.config.getint("POSITIONS", "camera.find.%d.A" % n)
			gripper = self.Head.MinS
			b = self.config.getint("POSITIONS", "camera.find.%d.B" % n)
			g = self.config.getint("POSITIONS", "camera.G")
			self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, a),
						   self.Head.SetServo(self.Head.DOF_GRIPPER, gripper),
						   self.Head.SetServo(self.Head.DOF_B, b),
						   self.Head.SetServo(self.Head.DOF_G, g)))
			return (b, a, g, gripper)
		else:
			return None

	def ObjToDict(self, obj, a):
		objdict = {"MinX": obj.MinX,
		           "MinY": obj.MinY,
		           "MaxX": obj.MaxX,
		           "MaxY": obj.MaxY,
		           "bb": obj.BorderBits,
		           "type": obj.ObjectType}
		if a in self.Views:
			camera_width = float(CameraGetValues()["width"])
			camera_height = float(CameraGetValues()["height"])
			cx = (obj.MaxX + obj.MinX) / 2.0
			cy = (obj.MaxY + obj.MinY) / 2.0
			py = self.Views[a]["H"] / camera_height
			px = (self.Views[a]["W2"] + cy * (self.Views[a]["W1"] - self.Views[a]["W2"]) / camera_height) / camera_width
			d = (camera_height - cy) * py + self.Views[a]["D"]
			dx = (cx - camera_width / 2.0) * px
			w = (obj.MaxX - obj.MinX) * px
			objdict["h"] = (obj.MaxY - obj.MinY) * py
			if d < self.DMIN:
				objdict["reach"] = "too close"
			else:
				if d > self.DMAX:
					objdict["reach"] = "too far"
				else:
					if w > self.WMAX:
						objdict["reach"] = "too big"
					else:
						if w < self.WMIN:
							objdict["reach"] = "too small"
						else:
							if self.WMAX - w >= 2 * abs(dx):
								objdict["reach"] = "can take"
							else:
								objdict["reach"] = "need adjust"
			objdict["w"] = w
			objdict["d"] = d
			objdict["dx"] = dx
			objdict["cx"] = cx
			objdict["cy"] = cy
			objdict["px"] = px
			objdict["py"] = py
		return objdict

	def CmdPreview(self, file):
		if self.CmdCameraFire(file) == self.FAIL:
			return self.FAIL
		result = {"camera":self.LastResult}

		a = self.Head.GetServo(self.Head.DOF_A)
		objects = []
		for i in range(self.VarCamera.NumObjects):
			obj = self.VarCamera.Objects[i]
			objects.append(self.ObjToDict(obj, a))
		result["objects"] = objects
		result["a"] = a
		self.SetResult(result)
		return self.SUCCESS

	def CmdLook2(self, file, pos = 0):
		self.SetLookPosition(pos)

		return self.CmdPreview(file)

	def CmdMoveToObj(self, dx, d):
		b = self.Head.GetServo(self.Head.DOF_B)
		new_b = b - 2500 * atan(dx / d) / pi
		self.Sleep(self.Head.SetServo(self.Head.DOF_B, new_b))
		result = {"new_b": new_b, "d": d, "dx": dx, "b": b}
		self.SetResult(result)
		return self.SUCCESS


	def MoveToObject(self):
		if self.CmdCameraFire(file) == self.FAIL:
			return {"error": "camera"}
		camera = self.LastResult
		
		if self.VarCamera.NumObjects == 0:
			return {"no_obects": {"camera": camera}}

		result = {"attempts":[]}
		attempt = 0
		total_attempts = int(self.config.get("POSITIONS", "find.attempts", "5"))

		a = self.Head.GetServo(self.Head.DOF_A)
		while True:
			notfit = []
			need_adjust = []
			take = None
			if self.VarCamera.NumObjects == 0:
				result["lost"] = {"camera": camera}
				break

			for i in range(self.VarCamera.NumObjects):
				obj = self.VarCamera.Objects[i]
				objdict = self.ObjToDict(obj, a)
				if objdict["reach"] == "can take":
					take = objdict
					break
				else:
					if objdict["reach"] == "need adjust":
						need_adjust.append(objdict)
					else:
						notfit.append(objdict)

			attempt_data = {"notfit": notfit,
			                "need_adjust": need_adjust,
			                "take": take,
			                "attempt": attempt,
			                "camera": camera}
			result["attempts"].append(attempt_data)

			if take is not None:
				result["final"] = take
				break

			attempt += 1
			if attempt >= total_attempts:
				result["exceed"] = attempt
				break

			d = None
			dx = 10000000000
			for objdict in need_adjust:
				if objdict["dx"] < dx:
					dx = objdict["dx"]
					d = objdict["d"]

			if d is None:
				break

			self.CmdMoveToObj(dx, d)

			attempt_data["move"] = self.LastResult

			if self.CmdCameraFire(file) == self.FAIL:
				result["error"] = "camera"
				break
			camera = self.LastResult

		return result

	def FindIterator(self, waitPickAttempts = 0):
		pos = 0
		waitPick = waitPickAttempts
		while self.SetLookPosition(pos) is not None:
			self.CmdPrint("Has pos %d" % pos)
			move = self.MoveToObject()
			if "final" in move:
				yield move
				if waitPick > 0:
					waitPick -= 1
					continue
			pos += 1
			waitPick = waitPickAttempts
		self.CmdPrint("Stop finding")
		return

	def CmdFindFirst(self):
		move = None
		for move in self.FindIterator():
			break
		self.SetResult(move)
		return self.SUCCESS

	def CmdSetGrabPosition(self, d, gripper):
		(a,g) = self.Learning.GetGrabPosition(d, gripper)
		self.Sleep(max(self.Head.SetServo(self.Head.DOF_GRIPPER, gripper),
		               self.Head.SetServo(self.Head.DOF_A, a),
		               self.Head.SetServo(self.Head.DOF_G, g)))
		result = {}
		result["d"] = d
		result["gripper"] = gripper
		result["a"] = a
		result["g"] = g
		self.SetResult(result)
		return self.SUCCESS

	def CmdGrabAt2(self, d):
		gripper = self.Head.MinS
		step = (self.Head.MaxS - self.Head.MinS) / 33.0
		result = {"d": d, "status": "LOST"}
		while gripper < self.Head.MaxS:
			if self.Head.GrabStatus() > 0:
				result["status"] = "TAKEN"
				break
			pos = self.Learning.GetGrabPosition(d, gripper)
			if pos is None:
				result["status"] = "OUT"
				break
			(new_a, new_g) = pos
			self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, new_a),
			               self.Head.SetServo(self.Head.DOF_G, new_g),
			               self.Head.SetServo(self.Head.DOF_GRIPPER, gripper)))
			gripper += step
		self.SetResult(result)
		return self.SUCCESS

	def CmdLift(self):
		self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, 2200),
		               self.Head.SetServo(self.Head.DOF_G, 3300)))
		return self.SUCCESS

	def CmdToBox(self):
		self.CmdLift()
		self.Sleep(self.Head.SetServo(self.Head.DOF_B, 400))
		self.Sleep(self.Head.SetServo(self.Head.DOF_GRIPPER, 0))
		self.Sleep(self.Head.SetServo(self.Head.DOF_B, 750))
		return self.SUCCESS

	def CmdFindAndGrabAll(self):
		result = []
		for move in self.FindIterator(waitPickAttempts = 10):
			obj = move["final"]
			a = self.Head.GetServo(self.Head.DOF_A)
			d = self.Learning.GetObjD(a, obj["cy"])
			move["a"] = a
			move["ld"] = d
			self.CmdGrabAt2(d)
			move["grab"] = self.LastResult
			if move["grab"]["status"] == "TAKEN":
				self.CmdToBox()
			result.append(move)
		self.SetResult(result)
		return self.SUCCESS

	def GetGrabPosition(self, type, d):
            return (self.Head.CountPredictionValue(self.ControlPrediction[type]["g"], d),
                    self.Head.CountPredictionValue(self.ControlPrediction[type]["a"], d))

	def CmdTestGrabPosition(self, type, d):
		(g, a) = self.GetGrabPosition(type, d)
		if type == 'OPEN':
			gripper = self.Head.MinS
		else:
			gripper = self.Head.MaxS
		self.Sleep(self.Head.SetServo(self.Head.DOF_GRIPPER, gripper))
		self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, a),
		               self.Head.SetServo(self.Head.DOF_G, g)))
		self.SetResult({"a": a, "g": g})
		return self.SUCCESS

	def CmdSetControlPoint(self, type, file):
		gripper = self.Head.GetServo(self.Head.DOF_GRIPPER)
		if gripper == self.Head.MinS:
			pos = "OPEN"
		else:
			if gripper == self.Head.MaxS:
				pos = "CLOSE"
			else:
				self.CmdPrint("Wrong gripper pos: %d" % gripper)
				return self.FAIL
		if type not in self.ControlPoints[pos]:
			self.CmdPrint("Wrong type: %s" % type)
			return self.FAIL
		self.ControlPoints[pos][type]["g"] = self.Head.GetServo(self.Head.DOF_G)
		self.ControlPoints[pos][type]["a"] = self.Head.GetServo(self.Head.DOF_A)

		if file:
			self.CmdIFind(file)
			if "d" in self.LastResult:
				self.ControlPoints['OEPN'][type]["d"] = self.LastResult["d"]
				self.ControlPoints['CLOSE'][type]["d"] = self.LastResult["d"]
		return self.SUCCESS

	def CountControlParameters(self):
		for pos,posval in self.ControlPoints.items():
                        values = posval["CLOSEST"].keys()
                        predictParams = self.ControlPrediction[pos].keys()
                        for param in predictParams:
                            values.remove(param)
                        if len(values) == 1:
                            predictValue = values[0]
                        else:
                            continue
                        results = {}
                        for param in predictParams:
                            results[param] = []
			d_positions = []
			for type,typeval in posval.items():
				if predictValue in typeval:
                                    d = 1
                                    dline = []
                                    for i in range(len(posval)):
					dline.append(d)
					d = d * typeval[predictValue]
                                    d_positions.append(dline)
                                    for param in predictParams:
                                        results[param].append(typeval[param])
				else:
                                    break
			for param in predictParams:
                            if len(posval) == len(results[param]):
				self.ControlPrediction[pos][param] = self.Head.CountPredictionParameters(d_positions, results[param])

	def CmdGetControlPoints(self):
		self.CountControlParameters()
		self.SetResult({"points":self.ControlPoints, "predict": self.ControlPrediction})
		return self.SUCCESS

	"""
		TOO FAR
		NOT TAKEN
		TAKEN
	"""

	def CmdIGrabAt(self, d):

		if d > self.ControlPoints["OPEN"]["FARTHEST"]["d"]:
			self.SetResult({"status": "TOO FAR"})
			return self.SUCCESS

		(g1, a1) = self.GetGrabPosition("OPEN",d)
		(g2, a2) = self.GetGrabPosition("CLOSE", d)

		STEPS = 33
		gripper = self.Head.MinS
		g = g1
		a = a1

		self.Sleep(self.Head.SetServo(self.Head.DOF_GRIPPER, gripper))

		gripper_step = float(self.Head.MaxS - self.Head.MinS) / STEPS
		g_step = float(g2 - g1) / STEPS
		a_step = float(a2 - a1) / STEPS

		result = {"g_step": g_step, "a_step": a_step, "status": "NOT TAKEN"}
		self.SetResult(result)

		for step in range(STEPS):
			if self.Head.GrabStatus() > 0:
				result["status"] = "TAKEN"
				break
			self.Sleep(max(self.Head.SetServo(self.Head.DOF_G, g),
			               self.Head.SetServo(self.Head.DOF_A, a),
			               self.Head.SetServo(self.Head.DOF_GRIPPER, gripper)))
			g = g + g_step
			a = a + a_step
			gripper = gripper + gripper_step
		return self.SUCCESS

	def CmdISafeLift(self):
		a = self.Head.GetServo(self.Head.DOF_A)
		g = self.Head.GetServo(self.Head.DOF_G)
		target_g = g + (a - self.Head.SAFE_A)
		if target_g > self.Head.MAX_G:
			self.Sleep(max(self.Head.SetServo(self.Head.DOF_G, self.Head.MAX_G),
			               self.Head.SetServo(self.Head.DOF_A, a - (self.Head.MAX_G - g))))
			self.Sleep(self.Head.SetServo(self.Head.DOF_A, self.Head.SAFE_A))
		else:
			self.Sleep(max(self.Head.SetServo(self.Head.DOF_G, target_g),
			               self.Head.SetServo(self.Head.DOF_A, self.Head.SAFE_A)))
		return self.SUCCESS

	"""
		TOO FAR
		NOT TAKEN
		DROPPED AFTER LIFT
		DROPPED AFTER TAKE0
		PLACED
	"""

	def CmdIGrabAtAndPlace(self, d, basketId):
		self.CmdIGrabAt(d)
		if self.LastResult["status"] == "TAKEN":
			self.CmdISafeLift()
			if self.Head.GrabStatus() == 0:
				self.LastResult["status"] = "DROPPED AFTER LIFT"
				return self.SUCCESS
			self.Sleep(self.Head.BasketTake0())
			if self.Head.GrabStatus() == 0:
				self.LastResult["status"] = "DROPPED AFTER TAKE0"
				return self.SUCCESS
			self.Sleep(self.Head.BasketTake(int(basketId)))
			self.Sleep(self.Head.Release())
			self.Sleep(self.Head.BasketBack(int(basketId)))
			self.LastResult["status"] = "PLACED"
		return self.SUCCESS

	def CmdIGrabFromAndPlace(self, basketId):
		#g supposed to be self.Head.MAX_G
		d = self.GetDistance()
		self.CmdIGrabAtAndPlace(d, basketId)
		self.LastResult["d"] = d
		return self.SUCCESS

	def CmpObjectsSize(self, a, b):
		return (b["MaxX"] - b["MinX"]) * (b["MaxY"] - b["MinY"]) - (a["MaxX"] - a["MinX"]) * (a["MaxY"] - a["MinY"])

	def CmdCameraIObjects(self, file):
		if self.CmdCameraFire(file) == self.FAIL:
			return self.FAIL
		camera = self.LastResult

		result = []
		for i in range(self.VarCamera.NumObjects):
			obj = self.VarCamera.Objects[i]
			result.append({"MinX": obj.MinX,
			               "MinY": obj.MinY,
			               "MaxX": obj.MaxX,
			               "MaxY": obj.MaxY,
			               "bb": obj.BorderBits,
                                       "type": obj.ObjectType})
		result.sort(lambda a,b: self.CmpObjectsSize(a, b))

		self.SetResult({"objects":result, "camera": camera})
		return self.SUCCESS

	def GetDistance(self):
            #g supposed to be self.MAX_G
            return self.Head.CountPredictionValue(self.ControlPrediction["D"]["d"], self.Head.GetServo(self.Head.DOF_A))
        
	def CmdIPreview(self, file):
		status = self.CmdCameraIObjects(file)
		self.LastResult["d"] = self.GetDistance()
		return status

	def ExtractOrient(self, obj, stageX, stageY):
		bbh = obj["bb"] & 3
		if bbh == 0:
			stageX["max"] = obj["MaxX"]
			stageX["min"] = obj["MinX"]
			stageX["shift"] = self.VarCamera.Width / 2.0 - (stageX["max"] + stageX["min"]) / 2.0
		else:
			if bbh == 1:
				stageX["max"] = obj["MaxX"]
				stageX["shift"] = self.VarCamera.Width * 0.9 - stageX["max"]
			else:
				if bbh == 2:
					stageX["min"] = obj["MinX"]
					stageX["shift"] = self.VarCamera.Width * 0.1 - stageX["min"]

		bbv = obj["bb"] & 12
		if bbv == 0:
			stageY["max"] = obj["MaxY"]
			stageY["min"] = obj["MinY"]
			stageY["shift"] = self.VarCamera.Height / 2.0 - (stageY["max"] + stageY["min"]) / 2.0
		else:
			if bbv == 4:
				stageY["max"] = obj["MaxY"]
				stageY["shift"] = self.VarCamera.Height * 0.9 - stageY["max"]
			else:
				if bbv == 8:
					stageY["min"] = obj["MinY"]
					stageY["shift"] = self.VarCamera.Height * 0.1 - stageY["min"]

	def CorrectExperience(self, stages, screen):
		last = len(stages) - 1
		if last >= 1:
			current = stages[last]
			while last >=1:
				last = last - 1
				prev = stages[last]
				if "max" in current and "max" in prev:
					if "min" in current and "min" in prev:
						current["actualShift"] = (current["max"] + current["min"]) / 2 - (prev["max"] + prev["min"]) / 2
						break
					else:
						current["actualShift"] = current["max"] - prev["max"]
						break
				else:
					if "min" in current and "min" in prev:
						current["actualShift"] = current["min"] - prev["min"]
						break
			if "actualShift" in current:
				current["BMove"] = prev["B"] - current["B"]
				current["BaseIdx"] = last
				if current["actualShift"] != 0 and current["BMove"] != 0:
					current["screenToAngleCounted"] = current["BMove"] * screen / current["actualShift"]
					current["diffPct"] = abs((current["screenToAngleCounted"] - current["screenToAngle"]) / current["screenToAngle"])
					if current["diffPct"] < 0.2:
						current["screenToAngle"] = current["screenToAngleCounted"]
						current["confidence"] = (1 + current["confidence"]) / 2.0
					else:
						if current["diffPct"] < 0.6:
							current["screenToAngle"] = (current["screenToAngle"] + current["screenToAngleCounted"]) / 2
							if current["diffPct"] > 0.4:
								current["confidence"] = (0.5 + current["confidence"]) / 2.0

	def CmdILook(self, file):
		result = {"X": [], "Y":[], "camera":[]}
		confidenceX = 1
		screenToAngleX = 1600
		confidenceY = 1
		screenToAngleY = 900
		B = self.Head.GetServo(self.Head.DOF_B)
		A = self.Head.MaxS - self.Head.GetServo(self.Head.DOF_A)
		stageX = None
		stageY = None
		for attempt in range(self.MAX_LOOK_ATTEMPTS):
			stageX = {"B": B,
			          "confidence": confidenceX,
			          "screenToAngle": screenToAngleX
			         }
			stageY = {"B": A,
			          "confidence": confidenceY,
			          "screenToAngle": screenToAngleY
			         }
			result["X"].append(stageX)
			result["Y"].append(stageY)

			if self.CmdCameraIObjects(None) == self.FAIL:
				result["complete"] = "CAMERA ERROR"
				break
			result["camera"].append(self.LastResult)

			if len(self.LastResult["objects"]) > 0:
				obj = self.LastResult["objects"][0]
				self.ExtractOrient(obj, stageX, stageY)
				if "shift" in stageX:
					stageX["shiftPct"] = stageX["shift"] / self.VarCamera.Width
					if abs(stageX["shiftPct"]) < 0.1:
						if ("min" in stageX) and ("max" in stageX):
							if "shift" in stageY:
								stageY["shiftPct"] = stageY["shift"] / self.VarCamera.Height
								if abs(stageY["shiftPct"]) < 0.05 and abs(stageX["shiftPct"]) < 0.05:
                                                                    result["complete"] = "SUCCESS"
                                                                    result["obj"] = obj
							else:
								result["complete"] = "GEOMETRY ERROR"
						else:
							result["complete"] = "OBJECT TOO BIG"
				else:
					result["complete"] = "OBJECT TOO BIG"
			else:
				result["complete"] = "NO OBJECTS"

			if "complete" in result:
				break

			self.CmdPrint("correction: start")

			self.CorrectExperience(result["X"], self.VarCamera.Width)
			self.CorrectExperience(result["Y"], self.VarCamera.Height)

			confidenceX = stageX["confidence"]
			screenToAngleX = stageX["screenToAngle"]
			confidenceY = stageY["confidence"]
			screenToAngleY = stageY["screenToAngle"]

			B = B - stageX["shiftPct"] * confidenceX * screenToAngleY
			if B < self.Head.MinS:
				B = self.Head.MinS
			else:
				if B > self.Head.MaxS:
					B = self.Head.MaxS

			if "shiftPct" in stageY:
				A = A - stageY["shiftPct"] * confidenceY * screenToAngleY
				if A < self.Head.MinS:
					A = self.Head.MinS
				else:
					if A > self.Head.MaxS:
						A = self.Head.MaxS

			self.Sleep(max(self.Head.SetServo(self.Head.DOF_B, B),
			               self.Head.SetServo(self.Head.DOF_A, self.Head.MaxS - A)))

		if "complete" not in result:
			result["complete"] = "OUT OF ATTEMPTS"
		
		if file:
			self.CmdCameraIObjects(file)

		self.SetResult(result)
		return self.SUCCESS

	def CmdTestLookPosition(self, a):
		wait = max(self.Head.SetServo(self.Head.DOF_A, a),
		           self.Head.SetServo(self.Head.DOF_G, self.Head.MAX_G))
		self.Sleep(wait)
		self.SetResult({"d": self.GetDistance()})
		return self.SUCCESS

	def CmdILookFromPosition(self, position):
		wait = max(self.Head.SetServo(self.Head.DOF_GRIPPER, 0),
		           self.Head.SetServo(self.Head.DOF_G, self.Head.MAX_G))
		for dof,value in position.items():
			wait = max(wait, self.Head.SetServo(dof, value))
		self.Sleep(wait)
		status = self.CmdILook(None)
		if self.LastResult["complete"] == "SUCCESS":
			self.LastResult["d"] = self.Head.GetDistance()
		return status

	def FindReset(self):
		self.FindPosition = 0

	def FindNextPosition(self):
		if self.FindPosition < len(self.FIND_POSITIONS):
			position = self.FIND_POSITIONS[self.FindPosition]
			self.FindPosition = self.FindPosition + 1
			return position
		return None

	def FindNext(self):
                self.CmdPrint("FindNext: pos=%d" % self.FindPosition)
		while True:
			position = self.FindNextPosition()
			if position:
				self.CmdILookFromPosition(position)
				if "d" in self.LastResult:
					return self.LastResult["d"]
			else:
				break
		return None

	def FindAgain(self):
		if self.FindPosition < len(self.FIND_POSITIONS):
			self.CmdILookFromPosition(self.FIND_POSITIONS[self.FindPosition])
			if "d" in self.LastResult:
				return self.LastResult["d"]
		return None

	def FindHaveInReachPositions(self):
		return self.FindPosition < self.IN_REACH_DISTANCE_POSITIONS

	def CmdIFind(self, file):
		self.CmdPrint("I-Find:")

		self.FindReset()
		result["d"] = self.FindNext()

		if file:
			self.CmdCameraIObjects(file)

		self.SetResult(result)
		return self.SUCCESS

	def CmdMoveTo(self, d, b):
		angleB = b - 2500
		if angleB < 0:
			self.CmdMoveRight(-angleB / 1000.0)
		else:
			self.CmdMoveLeft(angleB / 700.0)

		distanceMm = d - self.MOVE_D_TARGET
		self.CmdMoveDistance(distanceMm)

	def CmdIFindAndGrab(self):
		self.InitCameraNonBlocking()
		self.Sleep(max(self.Head.BasketBack(self.Head.BASKET_RIGHT),
		               self.Head.BasketBack(self.Head.BASKET_LEFT)))
		result = {"grabattempts": [], "status": "NO OBJECTS"}
		liftAttempts = 0
                pickAttempts = 0
		self.FindReset()
		grabattempt = {"d": self.FindNext()}
                farObjects = []
		while grabattempt["d"]:
                    grabattempt["look"] = self.LastResult
                    self.CmdPrint("attempt %d %d" % (pickAttempts,liftAttempts))
                    result["grabattempts"].append(grabattempt)
                    grabattempt["B"] = self.Head.GetServo(self.Head.DOF_B)
                    if grabattempt["look"]["obj"]["type"] == 1:
                        basket = 0
                    else:
                        basket = 1
                    self.CmdIGrabAtAndPlace(grabattempt["d"], basket)
                    grab = self.LastResult
                    grabattempt["grab"] = grab
                    self.CmdPrint("grab status = %s" % (grab["status"]))
			
                    #TOO FAR
                    #NOT TAKEN
                    #DROPPED AFTER LIFT
                    #DROPPED AFTER TAKE0
                    #PLACED

                    if grab["status"] == "TOO FAR":
			farObjects.append(grabattempt)
                    else:
                        if grab["status"] == "NOT TAKEN":
                            if pickAttempts < self.MAX_PICK_ATTEMPTS:
				pickAttempts = pickAttempts + 1
                                d = self.FindAgain()
                                if d:
                                    grabattempt = {"d": d}
                                    continue
                            else:
				grabattempt["status"] = "TOO MANY PICK ATTEMPTS"
				pickAttempts = 0
                        else:
                            if grab["status"] == "PLACED":
                                d = self.FindAgain()
                                if d:
                                    grabattempt = {"d": d}
                                    continue                               
                            else:
                                #DROPPED AFTER LIFT
                                #DROPPED AFTER TAKE0
                                if liftAttempts < self.MAX_LIFT_ATTEMPTS:
                                    liftAttempts = liftAttempts + 1
                                    pickAttempts = 0
                                    self.FindReset()
                                else:
                                    result["status"] = "TOO MANY LIFT ATTEMPTS"

                    if (not self.FindHaveInReachPositions()) and len(farObjects) > 0:
                        grabattempt = farObjects[0]
                        self.CmdMoveTo(grabattempt["d"], grabattempt["B"])
                        pickAttempts = 0
                        liftAttempts = 0
                        farObjects = []
                        self.FindReset()

                    grabattempt = {"d": self.FindNext()}
                        
		self.SetResult(result)
		return self.SUCCESS

	# --------------------- end of obsolete ICollect stuff

	"""
	from: 0, 5000, 2200, *
	
	2500: X=510..760    Y=220..460 X0=635
	2300: X=680..960    Y=200..450 X0=820  D=+185 R=1.08
	2000: X=910..1190   Y=130..420 X0=1050 D=+415 R=1.20
	1800: X=1050..1260* Y=100..370 X0=?    D=+540 R=1.30
	2700: X=360..580    Y=210..460 X0=470  D=-165 R=1.21
	3000: X=90..360     Y=150..400 X0=225  D=-410 R=1.22
	
	Move: from 0, 5000, 2200, 2500
	
	init: X=500..760   Y=210..450 X0=630
	0.2-> X=830..1100  Y=150..400 X0=965 D=+335 R=1675
	0.2<- X=510..760   Y=210..460 X0=635 D=-330 R=1650
	0.2<- X=340..590   Y=240..460 X0=465 D=-170 R= 850
	0.1-> X=520..770   Y=230..460 X0=645 D=+180 R=1800
	0.1-> X=630..880   Y=210..450 X0=755 D=+110 R=1100
	0.1-> X=700..960   Y=190..430 X0=830 D=+75  R= 750
	0.1-> X=760..1090  Y=150..420 X0=925 D=+95  R= 950
	0.3<- X=280..580   Y=220..450 X0=430 D=-495 R=1650
	0.1<- X=260..520   Y=220..460 X0=390 D=-40  R= 400
	0.4-> X=940..1260* Y=120..380 X0=?   D=+680 R=1700


	0.1<- R= 400
	0.1-> R= 750
	0.1-> R= 950
	0.1-> R=1100
	0.1-> R=1800

	0.2<- R= 850
	0.2<- R=1650
	0.2-> R=1675

	0.3<- R=1650

	0.4-> R=1700



	1 sec - 195mm    195
	2 sec 410 - 415  205..207
	1.5 310..315     207..210
	0.5 100mm        200
	0.1 = 15mm       150
	0.2 = 35-40mm    175..200
	0.4 80mm         200


	"""

	def CmdMoveDistance(self, distanceMm):
		moveSec = distanceMm / self.D_TO_MOVE
		if moveSec > 0:
			self.CmdMoveForward(moveSec)
		else:
			self.CmdMoveBack(-moveSec)

	def CmdTurn(self, timeSec):
		if timeSec < 0:
			self.CmdMoveRight(-timeSec)
		else:
			self.CmdMoveLeft(timeSec)

	def CmdMoveForward(self, timeSec):
		self.Head.MoveForward()
		self.Sleep(timeSec)
		self.Head.MoveStop()

	def CmdMoveBack(self, timeSec):
		self.Head.MoveBack()
		self.Sleep(timeSec)
		self.Head.MoveStop()

	def CmdMoveRight(self, timeSec):
		self.Head.MoveRight()
		self.Sleep(timeSec)
		self.Head.MoveStop()

	def CmdMoveLeft(self, timeSec):
		self.Head.MoveLeft()
		self.Sleep(timeSec)
		self.Head.MoveStop()

	"""
	Priorities:
		1. Consider reaching area:
		    1.1 Biggest full
		    1.2 Biggest partial
		2. Not reaching area - closest to farthest (maybe avoid tooo big)
	
	Properties:
		1. full/partial, there to move, best position by being taken
		2. distance by middle, size, center
		3. ? distance by min reachable, size, center
	"""
	def CmpObjPriority(self, a, b):
		if a["d"] >= self.Learning.DMIN and a["d"] <= self.Learning.DMAX:
			if b["d"] >= self.Learning.DMIN and b["d"] <= self.Learning.DMAX:
				if a["full"]:
					if b["full"]:
						return int(b["width"] - a["width"])
					else:
						-1
				else:
					if b["full"]:
						return 1
					else:
						return int(a["d"] - b["d"])
			else:
				return -1
		else:
			if b["d"] >= self.Learning.DMIN and b["d"] <= self.Learning.DMAX:
				return 1
			else:
				return int(a["d"] - b["d"])

	def CmdCameraSelectObject(self, file):
		if self.CmdCameraFire(file) == self.FAIL:
			return self.FAIL
		camera = self.LastResult

		a = self.Head.GetServo(self.Head.DOF_A)

		result = []
		for i in range(self.VarCamera.NumObjects):
			obj = self.VarCamera.Objects[i]
			(lc_x, lc_y) = self.Learning.D.GetValue((obj.MinX, obj.MinY, a))
			(rc_x, rc_y) = self.Learning.D.GetValue((obj.MaxX, obj.MinY, a))
			(lf_x, lf_y) = self.Learning.D.GetValue((obj.MinX, obj.MaxY, a))
			(rf_x, rf_y) = self.Learning.D.GetValue((obj.MaxX, obj.MaxY, a))

			center_x = (lc_x + rc_x + lf_x + rf_x) / 4
			center_y = (lc_y + rc_y + lf_y + rf_y) / 4
			result.append({"d": sqrt(center_x * center_x + center_y * center_y),
			               "width": (lc_x + lf_x - rc_x - rf_x) / 2,
			               "full": obj.BorderBits == 0,
			               "cx": center_x,
			               "minx": obj.MinX,
			               "maxx": obj.MaxX,
			               "miny": obj.MinY,
			               "maxy": obj.MaxY,
			               "bb": obj.BorderBits,
			               "rc_x": rc_x,
			               "rc_y": rc_y,
			               "lc_x": lc_x,
			               "lc_y": lc_y,
			               "rf_x": rf_x,
			               "rf_y": rf_y,
			               "lf_x": lf_x,
			               "lf_y": lf_y
			               })

		result.sort(lambda a,b: self.CmpObjPriority(a, b))

		self.SetResult({"objects":result, "camera": camera})
		return self.SUCCESS

	def CmdStartNavigate(self, file):
		self.Head.SetServo(self.Head.DOF_A, 4000)
		self.Head.SetServo(self.Head.DOF_B, (self.Head.MinS + self.Head.MaxS) / 2)
		self.Head.SetServo(self.Head.DOF_GRIPPER, self.Head.MinS)
		self.Sleep(self.Head.SetServo(self.Head.DOF_G, self.Head.MaxS))
		self.Sleep(1)
		self.CmdCameraFire(file)
		return self.SUCCESS

	"""
	def CmdMoveToView(self, x, y):
		(a, b, rx, ry) = self.Learning.GetAB(self.Head.GetServo(self.Head.DOF_A), self.Head.GetServo(self.Head.DOF_B), x, y)
		self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, a),
		               self.Head.SetServo(self.Head.DOF_B, b)))
		self.SetResult({"x": x, "y": y, "rx": rx, "ry": ry, "a": a, "b": b})
		return self.SUCCESS
	"""

	def CmdDLLook(self):
		self.InitCameraNonBlocking()

	def CmdMoveToViewAndPreview(self, x, y, file):
		self.CmdMoveToView(x, y)
		move = self.LastResult
		self.CmdCameraFire(file)
		camera = self.LastResult
		self.SetResult({"camera": camera, "move": move})
		return self.SUCCESS

	def CmdGrabAtXY(self, x, y):
		(a, b, rx, ry) = self.Learning.GetAB(self.Head.GetServo(self.Head.DOF_A), self.Head.GetServo(self.Head.DOF_B), x, y)
		d1 = sqrt(rx * rx + ry * ry)
		(a1, b1, rx0, d2) = self.Learning.GetAB(a, b, 160, 120)
		if d1 >= self.Learning.DMIN:
			if d1 <= self.Learning.DMAX:
				GR = self.Head.MinS
				(A, G) = self.Learning.G.GetValue([d1, GR])
				self.Sleep(max(self.Head.SetServo(self.Head.DOF_B, b),
							   self.Head.SetServo(self.Head.DOF_A, A)))
				while GR < self.Head.MaxS and self.Head.GrabStatus() == 0:
					(A, G) = self.Learning.G.GetValue([d1, GR])
					self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, A),
								   self.Head.SetServo(self.Head.DOF_G, G),
								   self.Head.SetServo(self.Head.DOF_GRIPPER, GR)))
					GR = GR + self.Learning.GRAB_STEP
				if self.Head.GrabStatus() > 0:
					self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, 1630),
								   self.Head.SetServo(self.Head.DOF_G, 4210)))
					if self.Head.GrabStatus() > 0:
						self.Sleep(self.Head.SetServo(self.Head.DOF_B, 400) + 0.5)
						self.Sleep(self.Head.SetServo(self.Head.DOF_GRIPPER, self.Head.MinS))
						status = "OK"
					else:
						status = "dropped"
				else:
					A = self.Learning.A.GetValue([d1])[0]
					self.Sleep(max(self.Head.SetServo(self.Head.DOF_A, A),
								   self.Head.SetServo(self.Head.DOF_G, 5000),
								   self.Head.SetServo(self.Head.DOF_GRIPPER, 0)))
					status = "not taken"
			else:
				status = "too far"
		else:
			status = "too close"
		self.SetResult({"x": x, "y": y, "rx": rx, "ry": ry, "a": a, "b": b, "d1": d1, "a1":a1, "b1":b1, "rx0": rx0, "d2": d2, "status": status})
		return self.SUCCESS

	def CmdLearnDLearning(self, tags, dumps):
		LearnExtractor(tags, dumps)
		return self.SUCCESS

	def CmdTestDump(self, dump, imgfile, mode):
		if self.InitCameraNonBlocking() == self.FAIL:
			self.CmdPrint("Init fail")
			return self.FAIL
		if imgfile:
			self.VarCamera.FileName = "static/%s" % imgfile
		else:
			self.VarCamera.FileName = None
		self.VarCamera.TestDump(dump, mode)
		return self.SUCCESS

	def CmdDeleteDump(self, dump):
		DeleteDump(dump)
		return self.SUCCESS
