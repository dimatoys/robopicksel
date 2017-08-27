import sys
sys.path.append('.')

from time import sleep
from Head import Head

head = Head()

head.MoveForward()

sleep(1)

head.MoveStop()


head.Shutdown()

