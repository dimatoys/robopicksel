import sys
sys.path.append('.')

from Camera import VisionInstance, InitCamera, LoadDump, ExtractorSet, CameraSetValues

class StdoutLogger:
	def debug(self, message):
		print message

def test_camera():
    from time import sleep, time

    #ExtractorSet({'':})
    """
	1475377543
	1475377592
	1475377611
	1475377636
	1475377658
    """
    CameraSetValues({'extractor':'statimg', 'DumpFile': '1477953421'})
    #CameraSetValues({'extractor':'statimg', 'DumpFile': '1477953409'})
    #CameraSetValues({'extractor':'statimg', 'DumpFile': '1477947527'})
    #CameraSetValues({'extractor':'statimg', 'DumpFile': '1477947546'})
    #ExtractorSet({'AreaCell': 15, 'AnomalyThreshold': 0.15})

    vision = VisionInstance()
    #sleep(2)
    vision.FileName = "static/test2.jpg"

    vision.Fire()
    vision.Release()
    
    objects = []
    for i in range(vision.NumObjects):
        obj = vision.Objects[i]
	objects.append({"MinX": obj.MinX,
			"MinY": obj.MinY,
			"MaxX": obj.MaxX,
			"MaxY": obj.MaxY,
			"bb": obj.BorderBits,
                        "type": obj.ObjectType})

    return "{'status': 'ok', 'status':%d, 'objects=%s'}" % (vision.Status, str(objects))

def test_local_camera2():
    from HeadLocal import HeadLocal
    from Commands import Commands
    g_Head = HeadLocal()
    logger = StdoutLogger()
    InitCamera("local", "dump", logger, g_Head)
    g_Commands = Commands(g_Head, logger)
    g_Commands.start()
    
    CameraSetValues({'DumpFile': '1477947546'})
    g_Commands.CmdIPreview('test3.jpg')
    
    g_Commands.Shutdown()

def test_serie():
    from time import sleep, time
    vision = VisionInstance()
    #sleep(2)
    #vision.FileName = "static/test2.jpg"

    #ExtractorSet({'':})

    for i in range(10):
        vision.Fire()
        print "%s + %s = %s" % (str(vision.FireStartWriteTime - vision.FireStartTime),
                                str(vision.FireEndTime - vision.FireStartWriteTime),
                                str(vision.FireEndTime - vision.FireStartTime))
    vision.Release()
    return "{'status': 'ok', 'status':%d}" % (vision.Status)

def test_local():
	from HeadLocal import HeadLocal
	InitCamera("local", "dump", StdoutLogger(), HeadLocal())
	#ExtractorSet({"Scale": "10"})
	print test_camera()

def test_remote():
	from Head import Head
	InitCamera("pi", "picamera", StdoutLogger(), Head())
	print test_serie()
	#print test_camera()

def test_brightness():
	InitCamera("pi", "cpp", StdoutLogger())
	from time import sleep, time
	vision = VisionInstance()
	sleep(2)
	#vision.SetCameraParameter("BRIGHTNESS",60)
	#vision.SetCameraParameter("CONTRAST",30)
	#vision.SetCameraParameter("SHARPNESS",0)
	#vision.SetCameraParameter("SATURATION",0)
	#vision.SetCameraParameter("EXPOSURE", 1);
	
	for b in range(50000,500000,50000):
		vision.FileName = "static/test2_SHUTTER_SPEED%d.jpg" % b
		print vision.FileName
		vision.SetCameraParameter("SHUTTER_SPEED",b)
		print vision.GetImageMetrics()
		#vision.Fire()
	vision.Release()

def readDump(fileName):

    from HeadLocal import HeadLocal
    InitCamera("local", "dump", StdoutLogger(), HeadLocal())
    (width, height, depth, data) = LoadDump(fileName)
    print (width, height, depth)

def test_dump():
	readDump("1477947546")

def testGaussians():
    from HeadLocal import HeadLocal
    from Commands import Commands
    g_Head = HeadLocal()
    logger = StdoutLogger()
    InitCamera("local", "dump", logger, g_Head)
    g_Commands = Commands(g_Head, logger)
    g_Commands.start()
    
    CameraSetValues({'extractor':'kp', 'DumpFile': '1477947546'})
    ExtractorSet({'GaussianSize': '5', 'Depth': '3'})
    g_Commands.CmdIPreview('test3.jpg')
    
    g_Commands.Shutdown()

def test_histogram():
    from HeadLocal import HeadLocal
    from Commands import Commands
    g_Head = HeadLocal()
    logger = StdoutLogger()
    InitCamera("local", "dump", logger, g_Head)
    g_Commands = Commands(g_Head, logger)
    #g_Commands.start()
    
    CameraSetValues({'DumpFile': '1502667084'})
    g_Commands.CmdCameraFire('green.jpg')
    
    g_Commands.ShutdownCamera()
    
    CameraSetValues({'DumpFile': '1502667129'})
    g_Commands.CmdCameraFire('blue.jpg')

    g_Commands.Shutdown()

def test_init():
	from HeadLocal import HeadLocal
	from Commands import Commands
	g_Head = HeadLocal()
	logger = StdoutLogger()
	InitCamera(logger, g_Head)
	g_Commands = Commands(g_Head, logger)
	g_Commands.CmdExtractorGetParameters()

def test_stat():
	from HeadLocal import HeadLocal
	from Commands import Commands
	g_Head = HeadLocal()
	logger = StdoutLogger()
	InitCamera(logger, g_Head)
	g_Commands = Commands(g_Head, logger)
	#g_Commands.CmdTestDump('1542258022', 't1.jpg', 1)
	#g_Commands.CmdTestDump('1542260639', 't1.jpg', 1)
	#g_Commands.CmdTestDump('1542260685', 't1.jpg', 1)
	#g_Commands.CmdTestDump('1542260719', 't1.jpg', 1)
	g_Commands.CmdTestDump('1542568320', 't1.jpg', 0)
	#g_Commands.CmdTestDump('1542568439', 't1.jpg', 0)

	g_Commands.ShutdownCamera()


#test_local()
#test_remote()
#test_brightness()
#test_dump()
#test_local_camera2()
#testGaussians()
#test_histogram()
test_stat()

