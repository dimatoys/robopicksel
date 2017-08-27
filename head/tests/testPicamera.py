import time
import picamera
import datetime

class TestStream:
    def write(self,b):
        size = len(b)
        print size
        return size

    def flush(self):
        print "flush"

orig_width = 2592   # 32 * 81
orig_height = 1944  # 16 * 121

#f = 16
#res = (orig_width / f, orig_height / f)

#res = (orig_width, orig_height)
res = (1280, 720)

# Produces a 3-dimensional RGB array from an RGB capture.
# Round a (width, height) tuple up to the nearest multiple of 32 horizontally
# and 16 vertically (as this is what the Pi's camera module does for                                                                                                       
# unencoded output).   

stream = TestStream()

def measure_capture(camera,num):
    global res
    camera.resolution = res
    start =  datetime.datetime.now()
    for n in range(num):
      #camera.capture(stream, format='rgb', resize=res, use_video_port=True)
      camera.capture(stream, format='rgb')
      #camera.capture(stream, format='rgb')
      end =  datetime.datetime.now()
      print (end - start) / (n + 1)

def measure_continuous(camera,num):
    start =  datetime.datetime.now()
    n = 0
    for a in camera.capture_continuous(stream, format='rgb', resize=res):
        end =  datetime.datetime.now()
        n = n + 1
        print (end - start) / n
        if n == num:
            break

print res
print res[0] * res[1] * 3

#with picamera.PiCamera() as camera:
camera = picamera.PiCamera()
#camera.start_preview()
# Camera warm-up time
time.sleep(2)
measure_capture(camera,3)
#measure_continuous(camera,10)
