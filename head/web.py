import sys

from flask import Flask
from flask import Response
from flask import render_template
from flask import request

import json

from Commands import Commands
from Camera import InitCamera, LoadDump, GetDumps, GetMetadata, MetadataSet

print "Run: web.py"

app = Flask(__name__)
g_Head = None
g_Commands = None

@app.route("/")
def main():
    return render_template('main.html', pins = g_Head.Pins)

@app.route("/dumps")
def dumpslist():
    dumps = GetDumps()
    return Response(json.dumps(dumps), content_type='text/plain; charset=utf-8')

@app.route("/dumps/<path>")
def dumps(path):
    dump = LoadDump(path)
    if dump:
        (width, height, depth, data) = dump
        return Response(json.dumps({"width": width, "height": height, "depth": depth, "data": map(ord, data)}), content_type='text/plain; charset=utf-8')
    else:
        return Response(json.dumps({}), content_type='text/plain; charset=utf-8')

@app.route("/metadata")
def metadata():
	global Metadata
	if "text" in request.form:
		id = MetadataSet(json.loads(request.form["text"]))
		return Response("{'file': %s}" % id, content_type='text/plain; charset=utf-8')
	else:	
		return Response(json.dumps(GetMetadata()), content_type='text/plain; charset=utf-8')

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
