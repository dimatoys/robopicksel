import RPi.GPIO as GPIO
from Adafruit_PWM_Servo_Driver import PWM
from HeadLocal import HeadLocal
import serial
import time
from math import *
import smbus
import datetime
#import wiringpi

'''
	ffmpeg -ss 00:00:04 -t 40 -i ../MVI_3219.MOV G_%04d.png

	
	24 f/s
	A down 18-32=14 -> 0.5833s
	A halfdown from top: 16-23=7 -> 0.29s
	A halfdown from mid: 21-28=7
	A halfup from bottom: 25-34=9 ->0.375s
	A halfup from mid: 30-39=9
	A up 18-36=18 -> 0.75s

	G halfdown from mid: 7-15=8 -> 0.3333s
	G up 88-103=15 -> 0.625s
	G down 242-257=15
'''

'''

import smbus
import time
from math import *

# https://store.invensense.com/Datasheets/invensense/RM-MPU-9150A-00-v3.0.pdf

bus = smbus.SMBus(1)
address = 0x68

def read_byte(adr):
    return bus.read_byte_data(address, adr)

def read_word(adr):
    high = bus.read_byte_data(address, adr)
    low = bus.read_byte_data(address, adr+1)
    val = (high << 8) + low
    return val

def read_word_2c(adr):
    val = read_word(adr)
    if (val >= 0x8000):
        return -((65535 - val) + 1)
    else:
        return val

def write_byte(adr, value):
    bus.write_byte_data(address, adr, value)

write_byte(0x6b, 0) # wake up

while True:
    x_out = read_word_2c(0x43)
    y_out = read_word_2c(0x45)
    z_out = read_word_2c(0x57)

    #bearing  = atan2(y_out, x_out)
    #if (bearing < 0):
    #    bearing += 2 * pi


    print x_out, y_out, z_out




'''

class TServo:
    def SetPosition(self, angle):
        self.Angle = angle

    def GetPosition(self):
        return self.Angle

class IIC:
    def init(self, busid, address):
        self.Address = address
        self.Bus = smbus.SMBus(busid)
 
    def read_byte(self, adr):
        return self.Bus.read_byte_data(self.Address, adr)
    
    def write_byte(self, adr, value):
        self.Bus.write_byte_data(self.Address, adr, value)

    def read_word(self, adr):
        high = self.read_byte(adr)
        low = self.read_byte(adr + 1)
        val = (high << 8) + low
        return val

    def read_int(self, adr):
        val = self.read_word(adr)
        if val >= 0x8000:
            return val - 0x10000
        else:
            return val        

class HMC5883(IIC):
    def __init__(self, busid, shiftX, shiftY, shiftZ):
        self.init(busid, 0x1e)
        self.Ready = False
        self.ShiftX = shiftX
        self.ShiftY = shiftY
        self.ShiftZ = shiftZ
        try:
            self.write_byte(0, 0b01110000) # Set to 8 samples @ 15Hz
            self.write_byte(1, 0b00100000) # 1.3 gain LSb / Gauss 1090 (default)
            self.write_byte(2, 0b00000000) # Continuous sampling
            self.Ready = True
        except:
            pass

    def getXYZ(self):
        if self.Ready:
            return (self.read_int(3) - self.ShiftX, self.read_int(5) - self.ShiftY, self.read_int(7) - self.ShiftZ)
        else:
            return (0,0,0)

    def getAngle(self):
        (x, y, z) = self.getXYZ()
        return atan2(y, x)

class MPU6050(IIC):
    def __init__(self, busid, shiftX, shiftY, shiftZ):
        self.init(busid, 0x68)
        self.Ready = False
        self.ShiftX = shiftX
        self.ShiftY = shiftY
        self.ShiftZ = shiftZ
        try:
            # https://store.invensense.com/Datasheets/invensense/RM-MPU-9150A-00-v3.0.pdf
            self.write_byte(0x6b, 0) # wake up
            self.Ready = True
        except:
            pass

    def getXYZ(self):
        return (self.read_int(0x43) - self.ShiftX, self.read_int(0x45) - self.ShiftY, self.read_int(0x47) - self.ShiftZ)

    def getAngle(self):
        (x, y, z) = self.getXYZ()
        return atan2(y, x)

# middle 2715

class Stepper:
    STEPPER_STEPS = [[GPIO.HIGH, GPIO.HIGH, GPIO.LOW,  GPIO.LOW],
                     [GPIO.LOW,  GPIO.HIGH, GPIO.HIGH, GPIO.LOW],
                     [GPIO.LOW,  GPIO.LOW,  GPIO.HIGH, GPIO.HIGH],
                     [GPIO.HIGH, GPIO.LOW,  GPIO.LOW,  GPIO.HIGH]]

    def __init__(self, io, pins):
        self.IO = io
        self.Pins = pins
        self.Position = 0
        self.Delay = 0.05
    
    def SendPosition(self):
        c = self.Position % 4
        self.IO.Write(self.Pins[0], self.STEPPER_STEPS[c][0])
        self.IO.Write(self.Pins[1], self.STEPPER_STEPS[c][1])
        self.IO.Write(self.Pins[2], self.STEPPER_STEPS[c][2])
        self.IO.Write(self.Pins[3], self.STEPPER_STEPS[c][3])
        
    def Move(self, steps):
        if steps != 0:
            dir = steps / abs(steps)
            for i in range(abs(steps)):
                self.Position = self.Position + dir
                self.SendPosition()
                time.sleep(self.Delay)
        return self.Position
    
    def SetPosition(self, pos):
        while pos != self.Position:
            if pos > self.Position:
                self.Position = self.Position + 1
            else:
                self.Position = self.Position - 1
            self.SendPosition()

class DCMotor:
    def __init__(self, io, pins):
        self.IO = io
        self.Pins = pins
        self.Stop()
        
    def Forward(self):
        self.IO.HIGH(self.Pins[0])
        self.IO.LOW(self.Pins[1])

    def Back(self):
        self.IO.LOW(self.Pins[0])
        self.IO.HIGH(self.Pins[1])

    def Stop(self):
        self.IO.LOW(self.Pins[0])
        self.IO.LOW(self.Pins[1])

class CompasMotor(TServo):
    def __init__(self, motor, staticCompass, dynamicCompass, compassShift):
        self.Motor = motor
        self.StaticCompass = staticCompass
        self.DynamicCompass = dynamicCompass
        self.CompassShift = compassShift
        
    def SetPosition(self, angle):
        super(TServo, self).SetPosition(angle)
    
    def GetRealPosition(self):
        angle = (self.DynamicCompass.getAngle() - (self.StaticCompass.getAngle() - self.CompassShift)) % (pi * 2)
        if angle < pi:
            return angle
        else:
            return angle - pi * 2

class A116:
    CMD_EEP_WRITE = 1
    CMD_EEP_READ = 2
    CMD_RAM_WRITE = 3
    CMD_RAM_READ = 4
    CMD_I_JOG = 5
    CMD_S_JOG = 6
    CMD_STAT = 7
    CMD_ROLLBACK = 8
    CMD_REBOOT = 9
    
    SET_POSITION_CONTROL = 0
    SET_SPEED_CONTROL = 1
    SET_TORQUE_OFF = 3
    SET_POSITION_CONTROL_SERVO_ON = 4

    def __init__(self):
        self.Port = None
        self.Current = None
        self.Open()

    def Open(self):
        if self.Port:
            return
        self.Port = serial.Serial("/dev/ttyAMA0",
                                  baudrate = 115200,
                                  parity=serial.PARITY_NONE,
                                  stopbits=serial.STOPBITS_ONE,
                                  bytesize=serial.EIGHTBITS,
                                  timeout = 1)

    def Send(self, id, cmd, data):
        self.Open()

        n = 7 + len(data)
        checksum = n ^ id ^ cmd

        for d in data:
            checksum = checksum ^ d

        c1 = checksum & 0xFE
        c2 = (~c1) & 0xFE
        send_data = [0xFF, 0xFF, n, id, cmd, c1, c2]
        send_data.extend(data)
        #print send_data

        self.Port.write(bytearray(send_data))

    def ISetPosition(self, id, goal, playTimeMs = None):
        print "SetPos=%d" % goal
        if not playTimeMs:
            if self.Current:
                playTimeMs = abs(self.Current - goal)
            else:
                playTimeMs = 1000
        self.Current = goal

        pt = int(playTimeMs / 10)
        if pt > 255:
            pt = 255

        self.Send(id, self.CMD_I_JOG, [goal & 0xFF,
                                       (goal / 0x100) & 0xFF,
                                       self.SET_POSITION_CONTROL,
                                       id,
                                       pt])

    def ITorqueOff(self, id):
        self.Send(id, self.CMD_I_JOG, [0,
                                       0,
                                       self.SET_TORQUE_OFF,
                                       id,
                                       0])

    def Close(self):
        if self.Port:
            self.Port.close()
            self.Port = None

    """
    def SendAndReceive(self, id, cmd, data):            
        self.Send(id, cmd, data)
        
        time.sleep(0.1)

        result = []
        while self.Port.inWaiting() > 0:
            bytes = self.Port.read()
            result.extend(bytes)
        return result        

    def GetStatus(self, id):
        answer = motor.SendAndReceive(id, self.CMD_STAT, [])
        if len(answer) == 17 and ord(answer[0]) == 0xFF and ord(answer[1]) == 0xFF:
            return {"status_error": ord(answer[7]),
                "status_detail": ord(answer[8]),
                "PWM": ord(answer[9]) + ord(answer[10]) * 0x100,
                "pos_ref": ord(answer[11]) + ord(answer[12]) * 0x100,
                "position": ord(answer[13]) + ord(answer[14]) * 0x100,
                "lbus": ord(answer[15]) + ord(answer[16]) * 0x100}
        else:
            print "len=", len(answer)
            for c in answer:
                print "%X %d" % (ord(c), ord(c))
        self.Close()
    """

import VL53L0X
class RangeVL53LOX:
	def __init__(self):
		self.tof = VL53L0X.VL53L0X()
		self.tof.start_ranging(VL53L0X.VL53L0X_BETTER_ACCURACY_MODE)

	def Shutdown(self):
		self.tof.stop_ranging()

	def GetDistance(self):
		return self.tof.get_distance()

class Head(HeadLocal):

    GPPins =[None, None, 3,  5,  7, 29, 31, 26, 24, 21, 19,
                   23,  32, 33,  8, 10, 36, 11, 12, 35, 38,
                   40,  15, 16, 18, 22, 37, 13]

    # max time of movement servo the whole amplitude
    SERVO_CYCLE = [0.2, 0.65, 0.75, 0.65, 1.0, 1.0]

    # constant delay for servo control
    SERVO_COMMAND_PAUSE = 0.1

    #sensors pins
    # sensor wires: YGBrBlW (7,8,25,G,24)
    Sensors = [24, 10]

    #motors pins
    MOTOR_IN1 = 9
    MOTOR_IN2 = 25
    MOTOR_IN3 = 18
    MOTOR_IN4 = 23

    LED = 7
    STEPPER_PINS = [27, 22, 4, 17]

    def __init__(self):
        super(Head, self).__init__()

        GPIO.setmode(GPIO.BCM)
        GPIO.setwarnings(False)

        self.PINS = [True, True, True, True, True, True, True, True, True, True, 
                     True, True, True, True, True, True, True, True, True, True, 
                     True, True, True, True, True, True, True, True, True, True]
                     
        self.Pins[4] = 'PWR'
        self.Pins[6] = 'PWR'
        
        self.Pins[self.GPPins[self.Sensors[0]]] = 'S-0R'
        self.Pins[14] = 'S-0R'
        
        self.Pins[self.GPPins[self.Sensors[1]]] = 'S-1L'
        self.Pins[20] = 'S-1L'

        """
        Calibration of center:
            r = min + (max - min) * center_value / 5000
        """
        self.intervals = [[0, 150, 330], [1, 230, 545], [2, 173, 627], [1, 0, 1024]]
        self.pwm = PWM(0x40, debug=True)
        if self.pwm.Device:
            self.pwm.setPWMFreq(60)
        self.Pins[1] = 'I2C1-VCC'
        self.Pins[3] = 'I2C1-SDA'
        self.Pins[5] = 'I2C1-SCL'
        self.Pins[9] = 'I2C1-GND'
        
        # x = -213..446  y = -844..-242
        #self.StaticCompass = HMC5883(1, 116.5, -543, 0)
        
        #self.StaticCompass = MPU6050(1, 0, 0, 0)
        # x = 11..601    y = -927..-387
        #self.DynamicCompass = HMC5883(0, 306, -657, 0)
        #self.Stepper = Stepper(self, self.STEPPER_PINS)
        self.LeftMotor = DCMotor(self, [self.MOTOR_IN1, self.MOTOR_IN2])
        self.Pins[self.GPPins[self.MOTOR_IN1]] = "M-IN1"
        self.Pins[self.GPPins[self.MOTOR_IN2]] = "M-IN2"
        
        self.RightMotor = DCMotor(self, [self.MOTOR_IN3, self.MOTOR_IN4])
        self.Pins[self.GPPins[self.MOTOR_IN3]] = "M-IN3"
        self.Pins[self.GPPins[self.MOTOR_IN4]] = "M-IN4"

        self.ServoB = A116()
        self.Pins[8] = 'TX'
        self.Pins[10] = 'RX'
        self.Pins[2] = 'A-5V'
        self.Pins[17] = 'A-3.3V'
        self.Pins[30] = 'A-GND'
        self.Pins[34] = 'A-GND'
        #self.HeadDCMotor = DCMotor(self, self.STEPPER_PINS[0:2])
        
        #self.HeadMotor = CompasMotor(self.HeadDCMotor, self.StaticCompass, self.DynamicCompass, 0)
                
        #self.Serial = serial.Serial('/dev/ttyAMA0', 9600, timeout = 1)
        #self.Serial.open()
                     
        self.LOW(self.LED)
        self.Pins[self.GPPins[self.LED]] = 'LED'
        self.Pins[25] = 'LED'
        
    def Read(self, id):
        if self.PINS[id]:
            GPIO.setup(id, GPIO.IN, GPIO.PUD_UP)
            self.PINS[id] = False
        return GPIO.input(id)

    def Write(self, id, value):
        if self.PINS[id]:
            GPIO.setup(id, GPIO.OUT)
            self.PINS[id] = False
        GPIO.output(id, value)
        
    def HIGH(self, id):
        self.Write(id, GPIO.HIGH)

    def LOW(self, id):
        self.Write(id, GPIO.LOW)

    def Shutdown(self):
        if self.pwm.Device:
            self.pwm.setAllPWM(0, 0)
        self.ServoB.Close()
        GPIO.cleanup()

    # return time of the movement
    def SetServo(self, id, s):
        
        if id >= len(self.intervals):
            return 0
        
        if s < self.MinS:
            s = self.MinS
        else:
            if s > self.MaxS:
                s = self.MaxS

        old_value = self.GetServo(id)
        super(Head, self).SetServo(id,s)

        pin = self.intervals[id][0]
        minServo = self.intervals[id][1]
        maxServo = self.intervals[id][2]

        value = int((s - self.MinS) * (maxServo - minServo ) / (self.MaxS - self.MinS) + minServo)
        if id == 3:
            #self.HeadMotor.SetPosition(value)
            self.ServoB.ISetPosition(pin, value)
        else:
            if self.pwm.Device:
                self.pwm.setPWM(pin, 0, value)
            else:
                print "setPWM(%d,0,%d)" % (pin,value)
        #self.ExecuteCommand("P%d,%d" % (pin, value))
        return abs(s - old_value) * self.SERVO_CYCLE[id] / (self.MaxS - self.MinS) + self.SERVO_COMMAND_PAUSE
    
    def ReadSensors(self):
        result = []
        for s in self.Sensors:
            result.append(self.Read(s))
        return result
    
    def ReadCompass(self):
        #return {"static": {"a":self.StaticCompass.getAngle(),
        #                   "xyz": self.StaticCompass.getXYZ()},
        #        "dynamic": {"a": self.DynamicCompass.getAngle(),
        #                    "xyz": self.DynamicCompass.getXYZ()},
        #        "head": self.HeadMotor.GetRealPosition()}
        return {}

    def MoveForward(self):
        self.LeftMotor.Forward()
        self.RightMotor.Forward()

    def MoveBack(self):
        self.LeftMotor.Back()
        self.RightMotor.Back()

    def MoveRight(self):
        self.LeftMotor.Forward()
        self.RightMotor.Back()

    def MoveLeft(self):
        self.LeftMotor.Back()
        self.RightMotor.Forward()

    def MoveStop(self):
        self.LeftMotor.Stop()
        self.RightMotor.Stop()

    def SetLED(self, value):
        if value:
            self.HIGH(self.LED)
        else:
            self.LOW(self.LED)

    
    def ExecuteCommand(self, command):
        cmd = command[0]
        if cmd == 'R':
            pin = int(command[1:])
            return self.Read(pin)
        if cmd == 'H':
            pin = int(command[1:])
            return self.HIGH(pin)
        if cmd == 'L':
            pin = int(command[1:])
            return self.LOW(pin)
        if cmd == 'S':
            if command[1] == 'L':
                return self.Stepper.Move(1)
            else:
                return self.Stepper.Move(-1)
        if cmd == 'D':
            self.Stepper.Delay = float(command[1:])
            return self.Stepper.Delay
        if cmd == 'X':
            if command[1] == 'd':
                compas = self.DynamicCompass
            else:
                compas = self.StaticCompass
            num = int(command[2:])
            result = []
            for i in range(num):
                result.append(compas.getXYZ())
                time.sleep(0.3)
            return result
        if cmd == 'C':
            if command[1] == 'd':
                compas = self.DynamicCompass
            else:
                compas = self.StaticCompass
            num = int(command[2:])
            result = []
            for i in range(num):
                result.append(compas.getAngle() * 180 / pi)
                time.sleep(0.3)
            return result
        if cmd == 'P':
            d = command[1:].split(":")
            channel = int(d[0])
            value = int(d[1])
            if self.pwm.Device:
                self.pwm.setPWM(self.intervals[channel][0], 0, value)
            else:
                print "no pwm"
            return (channel, value)
        if cmd == 'J':
            value = int(command[1:])
            self.ServoB.ISetPosition(1, value)
            return value
        if cmd == 'M':
            d = command[1:].split(":")
            channel = int(d[0])
            result = {"c": channel}
            if d[1] != "":
                self.intervals[channel][1] = int(d[1])
                result["min"] = self.intervals[channel][1]
            if d[2] != "":
                self.intervals[channel][2] = int(d[2])
                result["max"] = self.intervals[channel][2]
            return result
        if cmd == 'B':
            times = command[2:].split(":")
            sleep = float(times[0])
            if len(times) == 1 or (len(times) > 1 and times[1].strip() == ""):
                if command[1] == 'L':
                    self.HeadDCMotor.Forward()
                else:            
                    self.HeadDCMotor.Back()
                time.sleep(sleep)
                self.HeadDCMotor.Stop()
                return {"sleep": sleep}
            after = float(times[1])
            measurements = []
            currentTime = 0
            startTime = datetime.datetime.now()
            measurements.append({"static": self.StaticCompass.getAngle(),
                                 "dynamic": self.DynamicCompass.getAngle(),
                                 "time": currentTime})
            if command[1] == 'L':
                self.HeadDCMotor.Forward()
            else:            
                self.HeadDCMotor.Back()
            
            while currentTime < sleep:
                currentTime = (datetime.datetime.now() - startTime).total_seconds()
                measurements.append({"static": self.StaticCompass.getAngle(),
                                     "dynamic": self.DynamicCompass.getAngle(),
                                     "time": currentTime})
            self.HeadDCMotor.Stop()
            
            while currentTime < sleep + after:
                currentTime = (datetime.datetime.now() - startTime).total_seconds()
                measurements.append({"static": self.StaticCompass.getAngle(),
                                     "dynamic": self.DynamicCompass.getAngle(),
                                     "time": currentTime})
            
            return {"sleep": sleep, "measurements": measurements}
