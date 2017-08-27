import sys
sys.path.append('.')

from Commands import Commands
from HeadLocal import HeadLocal

class Logger:
	def debug(self, message):
		print(message)

logger = Logger()
g_Head = HeadLocal()
g_Commands = Commands(g_Head, logger)

def test1():
	global g_Commands
	g_Commands.CmdGetControlPoints()

	print "\n\n-----\n\n"
	print(g_Commands.LastResult['points']['OPEN']['FARTHEST'])
	print(g_Commands.LastResult['points']['OPEN']['MID'])
	print(g_Commands.LastResult['points']['OPEN']['CLOSEST'])
	print("\n")
	print(g_Commands.LastResult['predict']['OPEN']['a'])
	print(g_Commands.LastResult['predict']['OPEN']['g'])
	print "\n\n-----\n\n"

	g_Commands.CmdTestGrabPosition('OPEN', 164)

	print "\n\n-----\n\n"
	print(g_Commands.LastResult)

def test2():
	global g_Head
	a = [[1, 3415, 11662225], [1, 2491, 6205081], [1, 2202, 4848804]]
	results = [219.85, 106, 164]
	p = g_Head.CountPrediction(a, results)
	
	print a[0][0] * p[0] + a[0][1] * p[1] + a[0][2] * p[2] 
	print a[1][0] * p[0] + a[1][1] * p[1] + a[1][2] * p[2] 
	print a[2][0] * p[0] + a[2][1] * p[1] + a[2][2] * p[2] 

def test3():
	global g_Head
	a_positions = [[1, 2, 4], [1, 3, 9], [1, 4, 16]]
	results = [17, 34, 57]
	print g_Head.CountPrediction(a_positions, results)

test1()