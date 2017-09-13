import sys

from flask import Flask
from flask import Response
from flask import render_template
from flask import request

import json

from Commands import Commands
from Camera import InitCamera

print "Run: web.py"

app = Flask(__name__)
g_Head = None
g_Commands = None

@app.route("/")
def main():
    return render_template('main.html',
                           GM = g_Head.M,
                           GM2 = g_Head.M2,
                           GD = g_Head.D,
                           GH = g_Head.H,
                           GC = g_Head.C,
                           GR = g_Head.R,
                           CC = g_Head.CC,
                           CH = g_Head.CH,
                           CV = g_Head.CV,
                           SH = g_Head.SH,
                           GRH = g_Head.GRH,
                           MinS = g_Head.MinS,
                           MaxS = g_Head.MaxS,
                           pins = g_Head.Pins)

@app.route("/dumps/<path:path>")
def dumps(path):
    return send_from_directory('../dumps', path)

@app.route("/servo/<id>/<v>")
def servo(id, v):
    value = g_Head.SetServo(int(id), int(v))
    return Response("{'status': 'ok', 'value': %d}" % value, content_type='text/plain; charset=utf-8')

@app.route("/sensors")
def read_sensors():

    response = {"status": "ok",
                "sensors": g_Head.ReadSensors(),
                "servos": g_Head.ReadServos()}

    return Response(json.dumps(response), content_type='text/plain; charset=utf-8')

@app.route("/geometry/<x>/<b>/<z>")
def geometry(x, b, z):
    g_Head.MoveXZ(int(x), int(z))
    g_Head.SetB(int(b))
    servos = g_Head.ReadServos()
    return Response("{'status': 'ok', 'servos': [%d, %d, %d, %d]}" % (servos[0], servos[1], servos[2], servos[3]),
        content_type='text/plain; charset=utf-8')

@app.route("/command/<cmd>")
def command(cmd):
    status = g_Commands.Start(cmd)
    return Response("{'status': '%s'}" % {True: "ok", False: "busy"}[status] , content_type='text/plain; charset=utf-8')

@app.route("/cmdstatus")
def cmdstatus():
	response = {}
	if g_Commands.Cmd is None:
		response['status'] = g_Commands.LastStatus
		if g_Commands.LastResult is not None:
			response['result'] = g_Commands.LastResult
	else:
		response['status'] = g_Commands.Cmd
	
	return Response(json.dumps(response), content_type='text/plain; charset=utf-8')

@app.route("/shutdown")
def shutdown():
    app.logger.debug('SHUTDOWN START')
    g_Head.Shutdown()
    g_Commands.Shutdown()
    app.logger.debug('SHUTDOWN OK')
    shutdown_server = request.environ.get('werkzeug.server.shutdown')
    if shutdown_server is None:
        raise RuntimeError('Not running with the Werkzeug Server')
    shutdown_server()
    return Response("{'status': 'ok'}", content_type='text/plain; charset=utf-8')

if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "local":
        from HeadLocal import HeadLocal
        g_Head = HeadLocal()
        #InitCamera("local", "cpp", app.logger)
        InitCamera("local", "dump", app.logger, g_Head)
    else:
        from Head import Head
        g_Head = Head()
        #InitCamera("pi", "cpp", app.logger)
        #InitCamera("pi", "dump", app.logger)
        InitCamera("pi", "picamera", app.logger, g_Head)

    g_Commands = Commands(g_Head, app.logger)

    g_Commands.start()

    app.run(host='0.0.0.0', port=7778, debug=True)
