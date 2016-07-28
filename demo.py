from omium import *
import porthole
from omegaToolkit import *


o = Omium.getInstance()
ui = UiModule.createAndInitialize().getUi()

porthole.initialize(4080, './hello.html')
ps = porthole.getService()
ps.setServerStartedCommand('loadUi()')

def loadUi():
    global img
    global p
    img = Image.create(ui)
    p = o.getPixels()
    img.setData(p)
    o.open('http://localhost:4080')
    onResize()

getDisplayConfig().canvasChangedCommand = 'onResize()'
def onResize():
    r = getDisplayConfig().getCanvasRect()
    o.resize(r[2], r[3])
    img.setSize(Vector2(r[2], r[3]))
    # flip image Y
    img.setSourceRect(0, r[3], r[2], -r[3])

porthole.broadcastjs("Hello()")