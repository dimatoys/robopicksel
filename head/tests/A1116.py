import sys
sys.path.append('.')

from Head import A116
from Head import Head

import threading

def test1():
    motor = A116()
    ID = 1

    try:
        while True:
            a = raw_input("value:")
            motor.ISetPosition(ID, int(a))
    finally:
        motor.Close()


class TA116(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.Motor = A116()
        self.ID = 1

    def run(self):
        try:
            while True:
                a = raw_input("value:")
                self.Motor.ISetPosition(self.ID, int(a))
        except:
            pass
        

def test2():
    t = TA116()
    
    t.start()
    t.join()
    #t.run()
    
    t.Motor.Close()

def test3():
    head = Head()

    try:
        while True:
            a = raw_input("command:")
            head.ExecuteCommand(a)
    except:
        pass

    head.Shutdown()



#test1()
#test2()
test3()