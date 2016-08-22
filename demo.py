from omium import *
import porthole
from overlay import *

o = Omium.getInstance()
ui = UiModule.createAndInitialize().getUi()

porthole.initialize(4080, './hello.html')
ps = porthole.getService()
ps.setServerStartedCommand('loadUi()')

def loadUi():
	global img
	global p
	img = Overlay()
	img.setAutosize(True)
	guifx = OverlayEffect()
	guifx.setShaders('overlay/overlay.vert', 'overlay/overlay-flipy.frag')
	img.setEffect(guifx)
	p = o.getPixels()
	img.setTexture(p)
	o.open('http://localhost:4080')
	onResize()

getDisplayConfig().canvasChangedCommand = 'onResize()'
def onResize():
    r = getDisplayConfig().getCanvasRect()
    o.resize(r[2], r[3])

porthole.broadcastjs("Hello()")