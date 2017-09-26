from ctypes import *
import time
import datetime
import os
import os.path
import json

'''
http://picamera.readthedocs.org/en/release-1.10/fov.html#camera-modes

# 	Resolution 	Aspect Ratio 	Framerates 	Video 	Image 	FoV 	Binning
1 	1920x1080 	16:9 	1-30fps 	x 	  	Partial 	None
2 	2592x1944 	4:3 	1-15fps 	x 	x 	Full 	None
3 	2592x1944 	4:3 	0.1666-1fps 	x 	x 	Full 	None
4 	1296x972 	4:3 	1-42fps 	x 	  	Full 	2x2
5 	1296x730 	16:9 	1-49fps 	x 	  	Full 	2x2
6 	640x480 	4:3 	42.1-60fps 	x 	  	Full 	4x4
7 	640x480 	4:3 	60.1-90fps 	x 	  	Full 	4x4

'''
g_VisionModule = None
g_ExtractionAlgorithm = {}
g_Logger = None
g_Mode = None
g_PiCamera = None
g_CameraParameters = None
g_Head = None

g_DumpTemplate = None

#CameraWidth = 1280
#CameraHeight = 720
#CameraMode = 4
CameraWidth = "320"
CameraHeight = "240"
CameraMode = "7"
CameraDefAlgorithm = "dlearning"

CameraViewAngleX = 0.9773843811168246
CameraViewAngleY = 0.5410520681182421

CameraRemoteDumpTemplate = "/home/pi/projects/dumps/%ld.dump"
CameraLocalDumpTemplate = "../dumps/%ld.dump"
SettingsFile = "settings.json"
MetadataDir="../metadata/"

#CameraLocalDump = "../dumps/1438146364.dump"
#CameraLocalDump = "../dumps/1436252235.dump"
#CameraLocalDump = "../dumps/1447654674.dump"
#CameraLocalDump = "../dumps/1462331398.dump"
#CameraDumpFile = "1462433222"
#CameraDumpFile = "1477542292"
CameraDumpFile = "1502667084"
#CameraDumpFile = "1477542307"

Settings = None
Metadata = None

def GetSettings():
	global Settings
	if Settings == None:
		Settings = SettingsManagement()
	return Settings.Data

def SaveSettings():
	global Settings
	if Settings:
		Settings.Save()

def GetMetadata():
	global Metadata
	if Metadata == None:
		Metadata = MetadataManagement()
	return Metadata.Data

def MetadataSave():
	global Metadata
	if Metadata:
		return Metadata.Save()
	else:
		return None

def MetadataSet(data):
	global Metadata
	if Metadata:
		return Metadata.Set(data)
	else:
		return None

def Print(message):
	global g_Logger
	if g_Logger:
		g_Logger.debug(message)
	else:
		print message

class ExtractorParameter(Structure):
	TYPE_INT = 0
	TYPE_FLOAT = 1

	_fields_ = [("Name", c_char_p),
	            ("Type", c_int)]

class TObjectsExtractor(Structure):
	_fields_ = [("Factory", c_void_p),
	            ("NumParameters", c_int),
	            ("Parameters", ExtractorParameter * 10)]

	def __init__(self, algorithm):
		global g_VisionModule
		if g_VisionModule:
			g_VisionModule.extractorInit(byref(self), algorithm)
			self.ParametersMap = {}
			Print("NumParams=%d" % self.NumParameters)
			for i in range(self.NumParameters):
				self.ParametersMap[self.Parameters[i].Name] = [{"type": "int"}, {"type": "float"}][self.Parameters[i].Type]

	def Set(self, values):
		global g_VisionModule
		if g_VisionModule:
			for param, value in values.items():
				if param in self.ParametersMap:
					Print("extr:%s = %s" % (param, value))
					if self.ParametersMap[param]["type"] == "int":
						g_VisionModule.extractorSetInt(byref(self), param, int(value))
					else:
						g_VisionModule.extractorSetDouble(byref(self), param, float(value))

	def Get(self):
		global g_VisionModule
		if g_VisionModule:
			values = {}
			for param, desc in self.ParametersMap.items():
				if desc["type"] == "int":
					values[param] = g_VisionModule.extractorGetInt(byref(self), param)
				else:
					values[param] = g_VisionModule.extractorGetDouble(byref(self), param)
			return values
		else:
			return None

class DetectedObject(Structure):
	_fields_ = [("MinX", c_int),
	            ("MinY", c_int),
	            ("MaxX", c_int),
	            ("MaxY", c_int),
	            ("Size", c_int),
	            ("BorderBits", c_int),
                    ("ObjectType", c_int)]

class Vision(Structure):
	_fields_ = [("ObjectsExtractor", POINTER(TObjectsExtractor)),
	            ("FileName", c_char_p),
	            ("Width", c_int),
	            ("Height", c_int),
	            ("Depth", c_int),
	            ("Status", c_int),
	            ("DumpTemplate", c_char_p),
	            ("DumpId", c_long),
	            ("NumObjects", c_int),
	            ("Objects", DetectedObject * 10)]

	def __init__(self):
		global g_VisionModule
		global CameraMode
		global g_Mode, g_PiCamera, g_CameraParameters
		if g_Mode == "dump":
			(self.Width, self.Height, self.Depth, self.Image) = LoadDump(g_CameraParameters["DumpFile"])
		else:
			self.Width = int(g_CameraParameters["width"])
			self.Height = int(g_CameraParameters["height"])
			self.Depth = 3 #int(g_CameraParameters["depth"])
			if g_Mode == "picamera":
				import picamera
				g_PiCamera = picamera.PiCamera(resolution = (self.Width, self.Height),
															 sensor_mode=int(CameraMode))
				g_PiCamera.awb_mode = g_CameraParameters["awb_mode"] = "auto"
				g_PiCamera.brightness = int(g_CameraParameters["brightness"])
				g_PiCamera.contrast = int(g_CameraParameters["contrast"])
				g_PiCamera.exposure_compensation = int(g_CameraParameters["exposure_compensation"])
				g_PiCamera.exposure_mode = g_CameraParameters["exposure_mode"]
				g_PiCamera.iso = int(g_CameraParameters["iso"])
				self.DumpTemplate = CameraRemoteDumpTemplate

		self.StartTime = time.time()
		self.ObjectsExtractor = pointer(GetExtractor())
		self.Status = 0

	def WaitInit(self):
		global g_CameraParameters
		if "warmup" in g_CameraParameters:
			while time.time() < self.StartTime + float(g_CameraParameters["warmup"]):
				time.sleep(0.05)

	# blocking command
	def Fire(self):
		self.WaitInit()
		global g_VisionModule
		global g_Mode
		self.FireStartTime = datetime.datetime.now()
		if g_VisionModule:
			if g_Mode == "dump":
				self.write(self.Image)
			else:
				#g_Head.SetLED(True)
				g_PiCamera.capture(self, format='rgb')
				g_Head.SetLED(False)
		self.FireEndTime = datetime.datetime.now()
		Print("Fire: Ok: %s" % str(self.FireEndTime - self.FireStartTime))

	def write(self, image):
		global g_VisionModule
		global g_Head
		g_Head.SetLED(False)
		g_VisionModule.streamStart(byref(self))
		self.FireStartWriteTime = datetime.datetime.now()
		Print("Fire: write: %s" % (str(self.FireStartWriteTime - self.FireStartTime)))
		g_VisionModule.writeImage(image)
		Print("Fire: write Ok")
		return len(image)

	def flush(self):
		pass

	def IsReady(self):
		return self.Status == 0

	def Release(self):
		global g_PiCamera
		if g_PiCamera is not None:
			g_PiCamera.close()

class SettingsManagement:
	def __init__(self):
		self.Load()

	def Load(self):
		global SettingsFile
		try:
			f = open(SettingsFile)
			self.Data = json.load(f)
			f.close()
		except:
			self.Data = {}

	def Save(self):
		global SettingsFile
		f = open(SettingsFile, "w+")
		f.write(json.dumps(self.Data, sort_keys=True, indent=2, separators=(',', ': ')))
		f.close()

class MetadataManagement:
	def __init__(self):
		self.Load()

	def Load(self):
		global MetadataDir
		settings = GetSettings()
		if "Metadata" in settings:
			fileName = settings["Metadata"]["file"]
			f = open(MetadataDir + fileName)
			if f:
				self.Data = json.load(f)
				f.close()
		else:
			settings["Metadata"] = {}
		self.Data = {}

	def Set(self, data):
		self.Data = data
		return self.Save()

	def Save(self):
		global MetadataDir
		settings = GetSettings()
		settings["Metadata"]["file"] = datetime.datetime.now().strftime("%Y%m%d_%H%m%S")
		f = open(MetadataDir + fileName, "w+")
		f.write(json.dumps(self.Data, sort_keys=True, indent=2, separators=(',', ': ')))
		f.close()
		SettingsSave()
		return settings["Metadata"]["file"]
		

def InitCamera(type, mode, logger, head):
	global g_VisionModule
	global CameraRemoteDumpTemplate
	global CameraLocalDumpTemplate
	global CameraDumpFile
	global CameraWidth
	global CameraHeight
	global CameraMode
	global CameraDefAlgorithm

	global g_Logger
	global g_Mode
	global g_CameraParameters
	global g_Head
	global g_DumpTemplate
	global Settings
	global Metadata

	g_Mode = mode
	g_CameraParameters = {}
	if type == "local":
		g_VisionModule = cdll.LoadLibrary('../visionModuleLocal/libvisionModule.so')
	else:
		g_VisionModule = cdll.LoadLibrary('visionModule/libvisionModule.so')

	if mode == "dump":
		g_DumpTemplate = {"local": CameraLocalDumpTemplate, "remote": CameraRemoteDumpTemplate}[type]
		g_CameraParameters["DumpFile"] = CameraDumpFile
	else:
		g_CameraParameters["width"] = CameraWidth
		g_CameraParameters["height"] = CameraHeight
		#g_CameraParameters["depth"] = "3"
		g_CameraParameters["warmup"] = "2"
		if mode == "picamera":
			#g_CameraParameters["camera_mode"] = CameraMode
			#analog_gain
			#awb_gains
			g_CameraParameters["awb_mode"] = "auto"
			g_CameraParameters["brightness"] = "50"
			g_CameraParameters["contrast"] = "0"
			#digital_gain
			g_CameraParameters["exposure_compensation"] = "0"
			g_CameraParameters["exposure_mode"] = "auto"
			g_CameraParameters["iso"] = "0"
	g_CameraParameters["extractor"] = CameraDefAlgorithm

	g_VisionModule.extractorInit.argtypes = [POINTER(TObjectsExtractor), c_char_p]
	g_VisionModule.extractorSetInt.argtypes = [POINTER(TObjectsExtractor), c_char_p, c_int]
	g_VisionModule.extractorSetDouble.argtypes = [POINTER(TObjectsExtractor), c_char_p, c_double]
	g_VisionModule.extractorGetInt.argtypes = [POINTER(TObjectsExtractor), c_char_p]
	g_VisionModule.extractorGetInt.restype = c_int
	g_VisionModule.extractorGetDouble.argtypes = [POINTER(TObjectsExtractor), c_char_p]
	g_VisionModule.extractorGetDouble.restype = c_double

	g_Logger = logger
	g_Head = head

def VisionInstance():
	return Vision()

def GetExtractor():
	global g_ExtractionAlgorithm
	global g_CameraParameters

	extractor = g_CameraParameters["extractor"]

	Print("Extractor: %s" % extractor)

	if extractor not in g_ExtractionAlgorithm:
		g_ExtractionAlgorithm[extractor] = TObjectsExtractor(extractor)
	return g_ExtractionAlgorithm[extractor]

def ExtractorSet(values):
	GetExtractor().Set(values)

def ExtractorGet():
	return GetExtractor().Get()

def ExtractorParmeters():
	return GetExtractor().ParametersMap

def CameraGetValues():
	global g_CameraParameters
	return g_CameraParameters

def CameraSetValues(values):
	global g_CameraParameters
	keys = g_CameraParameters.keys()
	for param, value in values.items():
		if param in keys:
			Print("cam:%s = %s" % (param, value))
			g_CameraParameters[param] = value

def CameraXToAngle(x):
	global g_CameraParameters
	global CameraViewAngleX
	return (float(x) / float(g_CameraParameters["width"]) - 0.5) * CameraViewAngleX

def CameraYToAngle(y):
	global g_CameraParameters
	global CameraViewAngleY
	return (float(y) / float(g_CameraParameters["height"]) - 0.5) * CameraViewAngleY

def GetDumps():
    global g_DumpTemplate
    result = []
    for f in os.listdir(os.path.dirname(g_DumpTemplate)):
        result.append(f.replace(".dump", ""))
    return result

def LoadDump(fileName):
	global g_DumpTemplate
	try:
		f = open(g_DumpTemplate % int(fileName), 'rb')
		h = bytearray(f.read(12))
		data = f.read()
		width = h[0] + 256 * (h[1] + 256 * (h[2] + 256 * h[3]))
		height = h[4] + 256 * (h[5] + 256 * (h[6] + 256 * h[7]))
		depth = h[8] + 256 * (h[9] + 256 * (h[10] + 256 * h[11]))
		return (width, height, depth, data)
	except:
		return None

