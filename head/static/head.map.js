function AbstractMap(canvas, width, x0, y0) {

	return {
		Canvas: $(canvas),
		C: canvas.getContext("2d"),
		Width: width,
		X0: x0,
		Y0: y0,

		BackgroundColor: "#FFF",
		GridColor: "#000",
		GridStepMm: 50,

		Init: function() {
			this.W = this.Canvas.attr("width")
			this.H = this.Canvas.attr("height")
			this.Scale = this.W / this.Width
		},

		DrawGrid: function() {
			//draw background
			this.C.beginPath()
			this.C.fillStyle = this.BackgroundColor
			this.C.fillRect(0, 0, this.W, this.H)
			this.C.stroke()

			//draw grid
			this.Begin(this.GridColor, 1)
			this.C.strokeRect(0, 0, this.W, this.H)
			for (var n = 0; this.GetX(n) > 0; n += this.GridStepMm) {
				//alert("n=" + n + " x=" + this.GetX(n))
				this.C.moveTo(this.GetX(n), 0)
				this.C.lineTo(this.GetX(n), this.H)
				this.C.moveTo(this.GetX(-n), 0)
				this.C.lineTo(this.GetX(-n), this.H)
			}

			for(var n = this.GridStepMm; this.GetY(n) > 0; n += this.GridStepMm) {
				this.C.moveTo(0, this.GetY(n))
				this.C.lineTo(this.W, this.GetY(n))
			}
			this.End()
		},

		GetX: function(x) {
			return this.W * this.X0 + x * this.Scale
		},

		GetY: function(y) {
			return this.H * this.Y0 + y * this.Scale
		},

		Begin: function(color, lineWidth) {
			this.C.strokeStyle =color
			this.C.lineWidth = lineWidth
			this.C.beginPath()
		},

		End: function() {
			this.C.stroke()
		},

		MoveTo: function(x, y) {
			this.C.moveTo(this.GetX(x), this.GetY(y))
		},

		LineTo: function(x, y) {
			this.C.lineTo(this.GetX(x), this.GetY(y))
		},

		Circle: function(x, y, rPix) {
			var mx = this.GetX(x)
			var my = this.GetY(y)
			console.log("[" + x + ", " + y + "] -> [" + mx + ", " + my + "]")
			this.C.moveTo(mx + rPix, my)
			this.C.arc(mx, my, rPix, 0, 2 * Math.PI, false)
		},

		Line: function(x1, y1, x2, y2) {
			console.log("line [" + x1 + ", " + y1 + ", " + x2 + " ," + y2 + "] -> [" + this.GetX(x1) + ", " + this.GetX(y1) + ", " + this.GetX(x2) + " ," + this.GetX(y2) + "]")
			this.MoveTo(x1, y1)
			this.LineTo(x2, y2)
		}
	}
}

$(function() {

	$.widget( "head.topmap", {
		// default options
		options: {
			//backgroundColor: "#FFF",
			//gridColor: "#000",
			//gridStepMm: 50,
			headColor: "#88F",
			thickLineWidth: 5,
			viewAreaColor: "#F88",
			objectColor: "#000",

			MessageElement: undefined,

			//MinS
			//MaxS
			//GD
			//GM3
			//GH
			//GC
			//GR
			//CC
			//CH
			//CV

			updateB : function(b) {
				//$("#gb").val(b + MinS)
				//sendGeometry()
				alert("new B=" + b)
			},

			getServo: function(s) {
				//return $("#s" + s).val()
			}
		},

		// the constructor
		_create: function() {
			
			this.Map = AbstractMap(this.element[0], -(this.options.GM3 + this.options.GD * 2) * 2, 0.5, 1)
			this.Objects = []

			var thisOptions = this.options
			this.element.click(function(e){
				var pos = $(this).position()
				var w = $(this).attr("width")
				var h = $(this).attr("height")
				var cx = e.pageX - pos.left - w / 2
				var cy = h - (e.pageY-pos.top)
				var b = Math.floor((Math.atan(cy/cx)) * (thisOptions.MaxS - thisOptions.MinS) / Math.PI)
				if (b < 0) {
					b += thisOptions.MaxS - thisOptions.MinS
				}
				thisOptions.updateB(b + thisOptions.MinS)
			})
			this._refresh();
		},

		// called when created, and later when changing options
		_refresh: function() {
			this.InitGeometry()
			this.DrawHead()
			this.DrawViewArea()
			this.DrawObjects()
		},

		_destroy: function() {
		},
 
		// _setOptions is called with a hash of all options that are changing
		// always refresh when changing options
		_setOptions: function() {
			// _super and _superApply handle keeping the right this-context
			this._superApply( arguments );
			this._refresh();
		},

		redraw: function(objs) {
			this.Objects = objs
			this._refresh()
		},

		InitGeometry: function() {
			this.Map.Init()
			this.Map.DrawGrid()

			g = this.getServoAngle(1)
			a = this.getServoAngle(2)
			b = this.getServoAngle(3)

			sinA = Math.sin(a)
			cosA = Math.cos(a)
			sinGA = Math.sin(a + g)
			cosGA = Math.cos(a + g)

			this.GAC = g + a + this.options.CC
			this.SinB = Math.sin(b)
			this.CosB = Math.cos(b)

			this.HeadLength = this.options.GD + this.options.GD * cosA + this.options.GM3 * cosGA
			this.CameraDist = this.options.GD + this.options.GD * cosA + this.options.GC * cosGA + this.options.GR * sinGA

			// camera x, y, z position
			this.CX = this.CameraDist * this.SinB
			this.CY = this.CameraDist * this.CosB
			this.CZ = sinA * this.options.GD + sinGA * this.options.GC - cosGA * this.options.GR
		},

		DrawHead: function() {
			this.Map.Begin(this.options.headColor, this.options.thickLineWidth)
			this.Map.Line(0 ,0, this.HeadLength * this.SinB, this.HeadLength * this.CosB)
			this.Map.End()
		},

		DrawViewArea: function() {
			this.Map.Begin(this.options.viewAreaColor, 1)

			position = [0, 0]
			this.PositionOnFloor(0, this.options.CV / 2, position)
			this.Map.MoveTo(position[0], position[1])
			var bottom = Math.floor(position[1])
			this.PositionOnFloor(0, -this.options.CV / 2, position )
			this.Map.LineTo(position[0], position[1])
			var top = Math.floor(position[1])

			this.PositionOnFloor(this.options.CH / 2, 0, position)
			this.Map.MoveTo(position[0], position[1])
			var right = Math.floor(position[0])
			this.PositionOnFloor(-this.options.CH / 2, 0, position )
			this.Map.LineTo(position[0], position[1])
			var left = Math.floor(position[0])

			this.PositionOnFloor(this.options.CH / 2, this.options.CV / 2, position)
			this.Map.MoveTo(position[0], position[1])
			this.PositionOnFloor(-this.options.CH / 2, this.options.CV / 2, position )
			this.Map.LineTo(position[0], position[1])

			this.PositionOnFloor(this.options.CH / 2, -this.options.CV / 2, position)
			this.Map.MoveTo(position[0], position[1])
			this.PositionOnFloor(-this.options.CH / 2, -this.options.CV / 2, position )
			this.Map.LineTo(position[0], position[1])

			this.Map.End()

			if (this.options.MessageElement != undefined) {
				this.options.MessageElement.html("camera x = [" + left + " .. " + right + "] y = [" + bottom + " .. " + top + "]")
			}
		},

		DrawObjects: function() {
			this.Map.Begin(this.options.objectColor, this.options.thickLineWidth)
			for (var i in this.Objects) {
				if ("x" in this.Objects[i]) {
					this.Map.Circle(this.Objects[i].y, this.Objects[i].x, 5)
				}
				if (("ltx" in this.Objects[i]) && ("rtx" in this.Objects[i])) {
					this.Map.Line(this.Objects[i].lty, this.Objects[i].ltx, this.Objects[i].rty, this.Objects[i].rtx)
				}
				if (("rtx" in this.Objects[i]) && ("rbx" in this.Objects[i])) {
					this.Map.Line(this.Objects[i].rty, this.Objects[i].rtx, this.Objects[i].rby, this.Objects[i].rbx)
				}
				if (("rbx" in this.Objects[i]) && ("lbx" in this.Objects[i])) {
					this.Map.Line(this.Objects[i].rby, this.Objects[i].rbx, this.Objects[i].lby, this.Objects[i].lbx)
				}
				if (("lbx" in this.Objects[i]) && ("ltx" in this.Objects[i])) {
					this.Map.Line(this.Objects[i].lby, this.Objects[i].lbx, this.Objects[i].lty, this.Objects[i].ltx)
				}
			}
			this.Map.End()
		},

		PositionOnFloor: function(ax, ay, position) {
			var singacy = Math.sin(this.GAC + ay)
			var cosgacy = Math.cos(this.GAC + ay)
			if (singacy > 0) {
				directDistanceTo0 = (this.options.GH - this.CZ) / singacy
				distanceTo0 = directDistanceTo0 * cosgacy
				distanceToX = directDistanceTo0 * Math.tan(ax)
				position[0] = this.CX + distanceTo0 * this.SinB + distanceToX * this.CosB
				position[1] = this.CY + distanceTo0 * this.CosB - distanceToX * this.SinB
				return true
			}
			return false
		},

		servoToAngle: function(s) {
			return (s - (this.options.MaxS + this.options.MinS) / 2) * Math.PI / (this.options.MaxS - this.options.MinS)
		},

		getServoAngle: function(s) {
			return this.servoToAngle(this.options.getServo(s))
		}
	})
})