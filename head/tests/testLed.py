import sys
sys.path.append('.')

from time import sleep
from Head import Head

num = 10
pause = 3

head = Head()

for n in range(num):
	head.SetLED(True)
	sleep(pause)

	head.SetLED(False)
	sleep(pause)

head.Shutdown()
