from math import *
import numpy as np
from PIL import Image, ImageDraw
import re

class MinResolve:
    
    def Reset(self, x0, minStep):
        self.X0 = x0
        self.MinStep = minStep
        self.Values = {}
        self.MakeAround1()
    
    def GetValue(self, x):
        tx = tuple(x)
        if tx in self.Values:
            return self.Values[tx]
        v = self.Target(x)
        self.Values[tx] = v
        return v
    
    def MakeAround1(self):
        self.Around1 = []
        self.D = len(self.X0)
        v = [-1] * self.D
        while True:
            self.Around1.append(list(v))
            i = 0
            while i < self.D:
                if v[i] == 1:
                    i = i + 1
                else:
                    for j in range(i):
                        v[j] = -1
                    v[i] = 1
                    break
            if i == self.D:
                break
    
    def GetMinAround(self, x0, s):
        min = None
        for a1 in self.Around1:
            x = []
            for i in range(self.D):
                x.append(x0[i] + s[i] * a1[i])
            v = self.GetValue(x)
            if (not min) or v < min:
                min = v
                xmin = x
        return (min, xmin)
    
    def ResolveSimple(self):
        xmin = self.X0
        min = self.GetValue(xmin)
        min0 = min
        
        while True:
            (min1, xmin1) = self.GetMinAround(xmin, self.MinStep)
            if min1 < min:
                min = min1
                xmin = xmin1
                #print min, len(self.Values)
            else:
                break
        return (xmin, min, min0)

class ResolveSymmetry(MinResolve):
    
    def __init__(self, spoints):
        self.SPoints =spoints
    
    def Transform(self, cx, t):
        x = cx[0]
        y = cx[1]

        x0 = t[0]
        y0 = t[1]
        a = t[2]
        sx = t[3]
        
        dx = x - x0
        dy = y - y0
        s = sin(a)
        c = cos(a)
        return (dx * c - dy * s + x0 + sx,
                dx * s + dy * c + y0)

    def TransformOpt(self, cx):
        return self.Transform(cx, self.XMin)
                
    def Target(self, v):

        sum = 0
        
        for p1,p2 in self.SPoints:
            (x1, y1) = self.Transform(p1, v)
            (x2, y2) = self.Transform(p2, v)
            sum = sum + (x1 + x2) * (x1 + x2) + (y1 - y2) * (y1 -y2)
        
        return sum

    def Resolve(self, x0, y0, a0, s0, sx, sy, sa, ss):
        self.Reset([x0, y0, a0, s0], [sx, sy, sa, ss])
        (self.XMin, self.Min, self.Min0) = self.ResolveSimple()
        return (self.XMin, self.Min, self.Min0)

class TModelPoly:
    def __init__(self, x):
        self.S = 0
        self.X = x
        self.C = [0] * len(x)
        self.V = [[1] * len(x)]

    def Next(self):
        #print self.C
        r = 1
        for i in range(len(self.C)):
            r = r * self.V[self.C[i]][i]

        if self.C[0] == self.S:
            self.S = self.S + 1
            self.V.append([])
            for i in range(len(self.C)):
                self.C[i] = 0
                self.V[self.S].append(self.V[self.S - 1][i] * float(self.X[i]))
            self.C[-1] = self.S
        else:
            if self.C[-1] > 0:
                self.C[-2] = self.C[-2] + 1
                self.C[-1] = self.C[-1] - 1
            else:
                i = -2
                while self.C[i] == 0:
                    i = i - 1
                self.C[i - 1] = self.C[i - 1] + 1
                self.C[-1] = self.C[i] - 1
                self.C[i] = 0
        return r

class TPolyRegression:
    # s - max power
    def __init__(self, s):
        self.S = s
        
    def GetPolyArray(self, x):
        pm = TModelPoly(x)
        v = []
        while pm.S <= self.S:
            v.append(pm.Next())
        return v
    
    def GenerateMX(self, x):
        from numpy.linalg import inv
        import numpy as np
        T = []
        for cp in x:
            T.append(self.GetPolyArray(cp))
        T = np.array(T)
        (cols, rows) = T.shape
        if rows > cols:
            raise BaseException("Not enough sample, need at least %d, provided %d" % (rows, cols))
        TT = T.transpose()
        return np.dot(inv(np.dot(TT, T)), TT)

    def NewY(self, MX, y):
        import numpy as np
        self.R = np.dot(MX, np.array(y))

    def Learn(self, x, y):
        import numpy as np
        MX = self.GenerateMX(x)
        self.NewY(MX, y)

    def PrepareX(self,x):
        import numpy as np
        return np.array([self.GetPolyArray(x)])

    def Predict(self, px):
        import numpy as np
        return np.dot(px, self.R)[0]

    def GetValue(self, x):
        return self.Predict(self.PrepareX(x))

class SampleImage:
    def __init__(self, file, picture):
        global pictures
        self.File = file
        self.Pic = picture
        self.Image = None
        self.Real = None
        self.A = self.Pic["A"]
        self.Img = self.Pic["img"]
        self.N = len(self.Img)
        self.MaxX = self.Pic["x"]
        self.MinX = -self.MaxX
        self.MinY = self.Pic["y"][0]
        self.MaxY = self.Pic["y"][-1]

    def GetImage(self):
        if not self.Image:
            self.Image = Image.open('VideoTest/%s' % self.File)
        return self.Image

    def DrawPixel(self, x, y, color):
        self.GetImage()
        x = int(x)
        y = int(y)
        if 0 <= x < self.Image.width and 0 <= y < self.Image.height:
            self.Image.putpixel((x, y), color)

    def DrawPointer(self, position, color):
        (x, y) = position
        self.DrawPixel(x, y, color)
        self.DrawPixel(x - 1, y, color)
        self.DrawPixel(x + 1, y, color)
        self.DrawPixel(x, y - 1, color)
        self.DrawPixel(x, y + 1, color)
        
    def Save(self, file, type):
        self.GetImage().save(file, type)
        
    def GetReal(self, i = None):
        if not self.Real:
            self.Real = []
            for x in [self.Pic["x"], 0, -self.Pic["x"]]:
                for y in self.Pic["y"]:
                    self.Real.append((x, y))
        if i != None:
            return self.Real[i]
        else:
            return self.Real
        
    def ResolveSymmetry(self):
        width = self.GetImage().width
        height = self.GetImage().height
        pr0 = TPolyRegression(2)
        pr0.Learn(self.Img, self.GetReal())
        points = []
        for i in range(self.N):
            (ix, iy) = self.Img[i]
            points.append((pr0.GetValue((ix, iy)), pr0.GetValue((width - ix , iy))))
        rs = ResolveSymmetry(points)
        print self.File, rs.Resolve(width/ 2, height / 2, 0, 0, 0.001, 0.001, 0.00001, 0.001)

        return rs

class TLearning:
    def __init__(self, config):

        self.config = config

        # (image_x, image_y, DOF_A) -> (x_distance, y_distance)
        self.D = TPolyRegression(3) 
        self.D.R = [[  4.43813268e+02,   1.06782991e+03],
            [ -2.16923364e-01,  -5.88883706e-01],
            [  9.06797661e-01,   3.93421281e+00],
            [ -2.92477465e+00,   5.82353218e-02],
            [  3.17365433e-05,   1.59439676e-04],
            [ -2.82137964e-04,  -2.38827075e-03],
            [  1.21844515e-04,   8.06557376e-03],
            [  1.36942251e-03,  -1.11000059e-05],
            [ -5.48744945e-03,  -1.00637749e-04],
            [  1.21304322e-03,  -2.26944837e-04],
            [ -2.31869648e-11,  -1.78206413e-08],
            [  1.90909436e-09,   3.91012915e-07],
            [  2.40234739e-08,  -2.04519518e-06],
            [ -2.01968782e-07,  -3.18965224e-06],
            [ -1.98306453e-07,  -7.04881662e-10],
            [  1.67199645e-06,  -6.66193280e-09],
            [ -8.05207388e-07,   2.54126171e-07],
            [ -4.69814641e-08,   4.07231531e-08],
            [ -2.46595579e-07,   2.92825242e-07],
            [ -2.22027845e-06,   1.53332454e-07]]

        # (y_distance) -> DOF_A
        self.A = TPolyRegression(3)
        self.A.R = [[  6.82446348e+03],
                    [ -1.94281251e+01],
                    [  2.81112033e-02],
                    [ -1.64016849e-05]]
        
        self.G = TPolyRegression(2)
        self.G.R = [[  8.90833181e+03,  -1.18804928e+03],
                    [  6.85736317e-02,  -2.14665285e-01],
                    [ -6.20407168e+01,   7.79806368e+01],
                    [  7.60782977e-05,  -1.01123014e-04],
                    [ -3.39912305e-03,   4.88348322e-03],
                    [  1.87343287e-01,  -2.65893282e-01]]
                    
        self.DMIN = 130
        self.DMAX = 230
        self.GRAB_STEP = 5000.0 / 33.0

    def LearnGrabPositions(self):
        i = 0
        x = []
        y = []
        self.MinD = 1000000
        self.MaxD = -1
        while self.config.has_option("POSITIONS", "D.%d" % i):
            D = self.config.getfloat("POSITIONS", "D.%d" % i)
            if D < self.MinD:
                self.MinD = D
            if D > self.MaxD:
                self.MaxD = D
            Aopen = self.config.getfloat("POSITIONS", "A.open.%d" % i)
            Gopen = self.config.getfloat("POSITIONS", "G.open.%d" % i)
            x.append([D, 0])
            y.append([Aopen, Gopen])

            Ahalf = self.config.getfloat("POSITIONS", "A.half.%d" % i)
            Ghalf = self.config.getfloat("POSITIONS", "G.half.%d" % i)
            x.append([D, 2500])
            y.append([Ahalf, Ghalf])

            Aclose = self.config.getfloat("POSITIONS", "A.close.%d" % i)
            Gclose = self.config.getfloat("POSITIONS", "G.close.%d" % i)
            x.append([D, 5000])
            y.append([Aclose, Gclose])

            i = i + 1

        self.Grab = TPolyRegression(2)
        self.Grab.Learn(x, y)

    def LearnCameraY(self):
        template = re.compile("camera\.y\.(\d+)\.(\d+)")
        Xc = {}
        Yc = {}
        X = []
        Y = []
        for p, v in self.config.items("POSITIONS"):
            m = template.match(p)
            if m:
                a = int(m.group(1))
                d = int(m.group(2))
                y = int(v)
                X.append([a, y])
                Y.append([d])
                if a not in Xc:
                    Xc[a] = []
                    Yc[a] = []
                Xc[a].append([y])
                Yc[a].append([d])
    
        self.CameraY = TPolyRegression(1)
        self.CameraY.Learn(X, Y)
    
        self.CameraYc = {}
        for a, x in Xc.items():
            self.CameraYc[a] = TPolyRegression(2)
            self.CameraYc[a].Learn(x, Yc[a])

    def GetGrabPosition(self, d, gripper):
        if d < self.MinD or d > self.MaxD:
            return None
        (a, g) = self.Grab.GetValue([d, gripper])
        return (a, g)

    def GetObjD(self, a, y):
        if a in self.CameraYc:
            d = self.CameraYc[a].GetValue([y])[0]
        else:
            d = self.CameraY.GetValue([a,y])[0]
        return d

    def GetAB(self, A, B, x, y):
        (xd, yd) = self.D.GetValue((x, y, A))
        d = sqrt(xd * xd + yd * yd)
        b = pi * 150.0 * (B - 2500) / 2500.0 / 180.0
        b = b + asin(xd / d)
        return (self.A.GetValue([d])[0], b * 180.0 * 2500.0 / pi / 150.0 + 2500, xd, yd)

    def CountA(self):
        ax = []
        ay = []
        for a in [3700, 3500, 3300, 3100, 2800, 2500, 2300, 2100]:
            (ox, oy) = self.D.GetValue((160, 120, a))
            ax.append([oy])
            ay.append([a])
        
        self.A = TPolyRegression(self.D.S)
        self.A.Learn(ax, ay)
        
    def DumpMatrix(self, M):
        result = "["
        for row in M:
            sep = "["
            for cell in row:
                result = "%s%s\t%f" % (result, sep, cell)
                sep = ","
            result = "%s],\n" % result
        print result

class ViewMap:

	def __init__(self, config):
		self.Config = config
		self.MapWidth = 100
		self.MapHeight = 100
		self.Scale = 0.2
		self.Map = []
		for y in range(self.MapHeight):
			self.Map.append([0] * self.MapWidth)

	def AddView(self, view, b):
		ba = (2500 - b) * pi / 2500
		bx1 = sin(ba) * view["D"]
		by1 = cos(ba) * view["D"]
		bx2 = sin(ba) * (view["D"] + view["H"])
		by2 = cos(ba) * (view["D"] + view["H"])
		cx = self.MapWidth / 2
		cy = self.MapHeight / 2
		points = []
		points.append((int((bx1 - cos(ba) * view["W1"] / 2) * self.Scale + cx), int((by1 + sin(ba) * view["W1"]) * self.Scale + cy)))
		points.append((int((bx1 + cos(ba) * view["W1"] / 2) * self.Scale + cx), int((by1 - sin(ba) * view["W1"]) * self.Scale + cy)))
		points.append((int((bx2 + cos(ba) * view["W2"] / 2) * self.Scale + cx), int((by2 - sin(ba) * view["W2"]) * self.Scale + cy)))
		points.append((int((bx2 - cos(ba) * view["W2"] / 2) * self.Scale + cx), int((by2 + sin(ba) * view["W2"]) * self.Scale + cy)))

		print("ba=%f bx1=%f by1=%f bx2=%f by2=%f cx=%f cy=%f" % (ba, bx1, by1, bx2, by2, cx,cy))
		print(str(points))

		p1 = None
		for i in range(4):
			(x, y) = points[i]
			self.Map[y][x] = 1
			if p1 is None or x < x1:
				x1 = x
				p1 = i

		min_right = p1 - 3
		min_diag = p1 - 2
		min_left = p1 - 1
		if points[min_right][0] > points[min_left][0]:
			p2 = min_left
			if points[min_right][0] > points[min_diag][0]:
				p3 = min_diag
				p4 = min_right
			else:
				p3 = min_right
				p4 = min_diag
		else:
			p2 = min_right
			if points[min_left][0] > points[min_diag][0]:
				p3 = min_diag
				p4 = min_left
			else:
				p3 = min_left
				p4 = min_diag

		x2 = points[p2][0]
		print(x1, x2)
		if x2 > x1:
			y0 = points[p1][1]
			s1 = float(points[min_right][1] - y0) / (points[min_right][0] - points[p1][0])
			s2 = float(points[min_left][1] - y0) / (points[min_left][0] - points[p1][0])
			if s1 > s2:
				st = s1
				s1 = s2
				s2 = st
			for x in range(x1, x2):
				y1 = y0 + int((x - x1) * s1)
				y2 = y0 + int((x - x1) * s2)
				for y in range(y1, y2 + 1):
					self.Map[y][x] = 1
		
		x3 = points[p3][0]
		if x3 > x2:
			for x in range(x2, x3):
				pass

		x4 = points[p4][0]
		if x4 > x3:
			pass

	def DrawMap(self, file_name):
		colors = [(0,0,0), (255,255,255)]
		img = Image.new('RGB', (self.MapWidth, self.MapHeight))
		for y in range(self.MapHeight):
			row = self.Map[y]
			for x in range(self.MapWidth):
				img.putpixel((x, self.MapHeight - y - 1), colors[row[x]])
		img.save(file_name)

class ViewRingMap:
	def __init__(self, config):
		self.Config = config
		self.PickViews = [3000, 2500]
		self.Views = {3000: {"D":  34.0, "H": 139.0, "W1": 185.0, "W2": 150.0},
		              2500: {"D": 104.0, "H": 132.0, "W1": 174.0, "W2": 180.0},
		              1500: {"D": 224.0, "H": 235.0, "W1": 198.0, "W2": 320.0}}

		self.Rings = {}

	def AddView(self, a, b):
		if a not in self.Views:
			return
		view = self.Views[a]
		ring = view["D"]
		ba = ba = (2500 - b) * pi / 2500
		if ring in self.Rings:
			rdata = self.Rings[ring]
		else:
			rdata = {"R": view["H"] + view["D"],
			         "W": atan(view["W2"] / 2.0 / (view["D"] + view["H"]))}
			self.Rings[ring] = rdata
		if "Left" not in rdata or ba - rdata["W"] < rdata["Left"]:
			rdata["Left"] = ba - rdata["W"]
		if "Right" not in rdata or ba + rdata["W"] > rdata["Right"]:
			rdata["Right"] = ba + rdata["W"]

	def DrawMap(self, file_name, size = 200):
		img = Image.new('RGB', (size, size), (0,0,0))
		ringsD = list(self.Rings.keys())
		if len(ringsD) > 0:
			ringsD.sort(reverse=True)
			draw = ImageDraw.Draw(img)
			maxring = self.Rings[ringsD[0]]
			scale = size / 2.0 / maxring["R"]
			center = size / 2
			for ring in ringsD:
				rdata = self.Rings[ring]
				outr = rdata["R"] * scale
				inr = ring * scale
				draw.pieslice([center - outr,
							   center - outr,
							   center + outr,
							   center + outr],
							   180 * (rdata["Left"] + 3 * pi / 2) / pi,
							   180 * (rdata["Right"] + 3 * pi / 2) /pi,
							   fill=(255, 255, 255))
				draw.ellipse([center - inr,
							  center - inr,
							  center + inr,
							  center + inr],
							  fill=(0, 0, 0))
		img.save(file_name)

	def GetNewPickView(self, b):
		ba = (2500 - b) * pi / 2500
		candidates = []
		for a in self.PickViews:
			ring = self.Views[a]["D"]
			if ring not in self.Rings:
				return (a, b)
			rdata = self.Rings[ring]
			if "Left" not in rdata:
				return (a, b)
			if ba >= rdata["Left"]:
				if ba <= rdata["Right"]:
					if rdata["Left"] > -pi / 2:
						candidates.append((a, rdata["Left"] - rdata["W"]))
					if rdata["Right"] < pi / 2:
						candidates.append((a, rdata["Right"] + rdata["W"]))
				else:
					if rdata["Right"] < pi / 2:
						candidates.append((a, rdata["Right"] + rdata["W"]))
			else:
				if rdata["Left"] > -pi / 2:
					candidates.append((a, rdata["Left"] - rdata["W"]))

		if len(candidates) == 0:
			return None

		new_b = None
		for a, b in candidates:
			if new_b is None or abs(ba - b) < abs(ba - new_b):
				new_a = a
				new_b = b

		return (new_a, 2500 - new_b * 2500 / pi)

class Geometry:

	DOF_GRIPPER = 0
	DOF_G = 1
	DOF_A = 2
	DOF_B = 3
	DOF_BASKET_R = 4
	DOF_BASKET_L = 5

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

	# max grabbing distance
	MAXX = sqrt((D + M3) * (D + M3) - H * H)

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

	# step pct of moving taking an object
	GRAB_STEP_PCT = 0.03

	# grab step pause
	GRAB_STEP_PAUSE = 0.1

	# safe distance from the floor, i.e. it is max object height
	SH = 35

	# distance from the floor to grab an object
	#GRH = 32
	GRH = 20

	# center of gripper
	GRS = 10

	# Camera X distance from DOF_A with a=0 g-corresponds to vertical
	CAMERA_R = 95.0
	# Camera distnce from the floor with a=0 g-corresponds to vertical
	CAMERA_C = 145.0

	SAFE_A = 1300
	MAX_G = 4745

	def __init__(self, head):
		self.Head = head
		self.GrabStep = (self.Head.MaxS - self.Head.MinS) * self.GRAB_STEP_PCT

	# return time
	def MoveXZ(self, x, z):
		x1 = x - self.D
		m3 = (self.M + self.M2) / 2
		s = x1 * x1 + z * z
		ss = sqrt(s)
		sinaf = (s + self.D * self.D - m3 * m3) / (2 * self.D * ss)
		if sinaf > 1:
			sinaf = 1.0
		else:
			if sinaf < -1:
				sinaf = -1.0
		if z > 0:
			f = asin(x1 / ss)
		else:
			f = pi - asin(x1 / ss)
		a = asin(sinaf) - f
		sinag = (z - self.D * sin(a)) / m3
		if sinag > 1:
			sinag = 1.0
		else:
			if sinag < -1:
				sinag = -1
		if self.D * cos(a) < x1:
			g = asin(sinag) - a
		else:
			g = (pi - asin(sinag)) - a

		time_a = self.SetAngle(self.DOF_A, a)
		time_g = self.SetAngle(self.DOF_G, g)
		return max(time_a, time_g)

	def SetB(self, b):
		self.Head.SetServo(self.DOF_B, b)
	
	def SetAngle(self, dof, angle):
		self.Head.SetServo(self.DOF_B, (self.Head.MaxS - self.Head.MinS) * (angle + pi) / pi / 2 + self.Head.MinS)

	def IsReleased(self):
		return self.Head.Servos[self.DOF_GRIPPER] == 0

	# return time
	def Release(self):
		return self.Head.SetServo(self.DOF_GRIPPER, 0)
	
	def GrabStatus(self):
		s = self.Head.ReadSensors()
		status = 0
		for sn in s:
			if sn == 0:
				status = status + 1
		return status

	def GetDistance(self):
		d_c = [1.99284446e-05, -1.33031509e-01, 3.03207967e+02]
		a = self.Head.GetServo(self.DOF_A)
		#g supposed to be self.MAX_G
		return ((d_c[0] * a) + d_c[1]) * a + d_c[2]

	def GetGrabOpenPosition(self, d):
		g1_c = [ -1.40679468e-01,   2.56832921e+01,   2.50716195e+03]
		a1_c = [  1.04290488e-01,  -2.90416544e+01,   4.59240029e+03]

		g1 = (g1_c[0] * d + g1_c[1]) * d + g1_c[2]
		a1 = (a1_c[0] * d + a1_c[1]) * d + a1_c[2]
		return (g1, a1)

	def GetGrabClosedPosition(self, d):
		g2_c = [ -8.19398315e-02,   1.60746743e+01,   3.65875576e+03]
		a2_c = [  8.86481772e-02,  -2.73472871e+01,   3.49261440e+03]
		g2 = (g2_c[0] * d + g2_c[1]) * d + g2_c[2]
		a2 = (a2_c[0] * d + a2_c[1]) * d + a2_c[2]
		return (g2, a2)

	BASKET_RIGHT = 0
	BASKET_LEFT = 1

	BASKET_POSITIONS = [{"dof": DOF_BASKET_R, "back": 4300, "take": 950, "B": 0},
	                    {"dof": DOF_BASKET_L, "back": 300, "take": 3865, "B": 5000}]

	def BasketBack(self, basketId):
		basket = self.BASKET_POSITIONS[basketId]
		return self.Head.SetServo(basket["dof"], basket["back"])

	def BasketTake0(self):
		time1 = self.Head.SetServo(self.DOF_G, 3570)
		time2 = self.Head.SetServo(self.DOF_A, 0)
		return max(time1, time2)

	def BasketTake(self, basketId):
		basket = self.BASKET_POSITIONS[basketId]
		time1 = self.Head.SetServo(basket["dof"], basket["take"])
		time2 = self.Head.SetServo(self.DOF_B, basket["B"])
		return max(time1, time2)

	def CountPrediction(self, positions, results):
		a = np.array(positions)
		b = np.array(results)
		x = np.linalg.solve(a, b)
		return x.tolist()
