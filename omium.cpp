#include <omega.h>
#include <omega/PythonInterpreterWrapper.h>

#include <cef_app.h>
#include <cef_client.h>
#include <cef_render_handler.h>
 #include <string> 

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class RenderHandler : public CefRenderHandler
{
public:
    RenderHandler(PixelData* pixels):
        myPixels(pixels)
    {
    }

public:
    bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
    {
        rect = CefRect(0, 0, myPixels->getWidth(), myPixels->getHeight());
        return true;
    }
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
    {
        byte* pixels = myPixels->map();
        memcpy(pixels, buffer, width*height * 4);
        myPixels->unmap();
        myPixels->setDirty();
    }

private:
    PixelData* myPixels;

public:
    IMPLEMENT_REFCOUNTING(RenderHandler);

};

///////////////////////////////////////////////////////////////////////////////
class BrowserClient : public CefClient
{
public:
    BrowserClient(RenderHandler *renderHandler)
        : myRenderHandler(renderHandler)
    {  }

    virtual CefRefPtr<CefRenderHandler> GetRenderHandler() 
    {
        return myRenderHandler;
    }

    CefRefPtr<CefRenderHandler> myRenderHandler;
public:
    IMPLEMENT_REFCOUNTING(BrowserClient);
};

///////////////////////////////////////////////////////////////////////////////
class BrowserApp : public CefApp
{
public:
    virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr< CefCommandLine > command_line)
    {
        // Enable gpu on child process
        command_line->AppendSwitch("disable-gpu");
    }

    IMPLEMENT_REFCOUNTING(BrowserApp);
};

///////////////////////////////////////////////////////////////////////////////
class Omium : public EngineModule
{
public:
    static Omium* instance;
    static Omium* getInstance() { return instance; }

public:
    Omium() : EngineModule("omium"),
        myInitialized(false),
        myFocused(false),
        myZoomLevel(1)
    {
        ModuleServices::addModule(this);
        setPriority(EngineModule::PriorityHigh);
        resize(600, 400);
        initialize();
    }
    ~Omium()
    {
        if(myInitialized)
        {
            myBrowser = NULL;
            myBrowserClient = NULL;
            myRenderHandler = NULL;
            CefShutdown();
        }
    }

    void open(const char* url)
    {
        if(myBrowser != NULL)
        {
            myBrowser->GetMainFrame()->LoadURL(url);
        }
        else
        {
            CefWindowInfo window_info;
            CefBrowserSettings browserSettings;
            browserSettings.webgl = STATE_DISABLED;

            RenderHandler* renderHandler = new RenderHandler(myPixels);
            //browserSettings.windowless_frame_rate = 60; // 30 is default
             
            // in linux set a gtk widget, in windows a hwnd. If not available set nullptr - may cause some render errors, in context-menu and plugins.
            //std::size_t windowHandle = 0;
            //renderSystem->getAutoCreatedWindow()->getCustomAttribute("WINDOW", &windowHandle);
            window_info.SetAsWindowless(NULL, true);
            myBrowserClient = new BrowserClient(renderHandler);
            myBrowser = CefBrowserHost::CreateBrowserSync(window_info, myBrowserClient.get(), url, browserSettings, NULL);
            myBrowser->GetHost()->SetZoomLevel(myZoomLevel);
        }
    }

    virtual void initializeRenderer(Renderer* r);

    virtual void update(const UpdateContext& context)
    {
        CefDoMessageLoopWork();
    }

    virtual void handleEvent(const Event& e)
    {
        if(myBrowser != NULL)
        {
			CefRefPtr<CefBrowserHost> host = myBrowser->GetHost();
            // Handle focus switches
            if(e.getServiceType() == Service::Pointer)
            {
				CefMouseEvent me;
				me.x = e.getPosition()[0];
				me.y = e.getPosition()[1];
				
				if(e.getType() == Event::Down)
				{
					// If we click on something opaque, the browser intercepts
					// this mouse event
					myPixels->beginPixelAccess();
					bool focusState = myPixels->getPixelA(me.x, me.y) > 0;
					myPixels->endPixelAccess();
					if(focusState != myFocused)
					{
						if(myFocusChangedCommand.size() > 0)
						{
							PythonInterpreter* pi = SystemManager::instance()->getScriptInterpreter();
							pi->queueCommand(myFocusChangedCommand);
						}
						myFocused = focusState;
					}
				}
				else if(e.getType() == Event::Move)
				{
					host->SendMouseMoveEvent(me, false);
				}
            }
					
            if(myFocused)
            {
				if(e.getServiceType() == static_cast<enum Service::ServiceType>(Event::ServiceTypeKeyboard))
				{
	            	e.setProcessed();
					uint sKeyFlags;
					//omsg("Keyboard Event");
					bool isDown = (e.getType() == Event::Down);
					// We assume the keyboard event contains the native scancode in the extra
					// data int array
				
					if(e.getExtraDataType() == Event::ExtraDataIntArray)
					{
						sendKeyEvent(e.getExtraDataInt(0), host, isDown);
					}
					char c;
					if(e.getChar(&c)) {
						sendCharEvent(c, (int)(c), host, e.getFlags());
					}

				} 
				else if(e.getServiceType() == Service::Pointer)
				{
					CefMouseEvent me;
					me.x = e.getPosition()[0];
					me.y = e.getPosition()[1];
					e.setProcessed();
					if(e.getType() == Event::Down || e.getType() == Event::Up)
					{
						CefBrowserHost::MouseButtonType mbt = MBT_LEFT;
						if(e.isFlagSet(Event::Right)) mbt = MBT_RIGHT;
						else if(e.isFlagSet(Event::Middle)) mbt = MBT_MIDDLE;

						host->SendMouseClickEvent(me, mbt, e.getType() == Event::Up, 1);
					}
				}
            }
        }
    }

    PixelData* getPixels() { return myPixels; }
    void sendCharEvent(char character, int code, CefBrowserHost * host, uint flags) {
        CefKeyEvent ce;
        ce.windows_key_code = code;
        ce.modifiers = flags;
        ce.character = character;
        ce.type = KEYEVENT_CHAR;
        host->SendKeyEvent(ce);
    }

    void sendKeyEvent(int code, CefBrowserHost * host, bool isDown) {
        CefKeyEvent ce;
        //ce.windows_key_code = code;
        ce.native_key_code = code;
        ce.focus_on_editable_field = true;

        if (isDown) {
            ce.type = KEYEVENT_KEYDOWN;
        } else {
            ce.type = KEYEVENT_KEYUP;
        }
        // HACK: only send keydown events (otherwise special key input like arrows
        // and backspace repeat)
        if(isDown) host->SendKeyEvent(ce);
    }

    void resize(int width, int height)
    {
        myWidth = width;
        myHeight = height;
        if(myPixels.isNull())
        {
            myPixels = new PixelData(PixelData::FormatBgra, myWidth, myHeight);
        }
        else
        {
            myPixels->resize(myWidth, myHeight);
        }

        if(myBrowser != NULL)
        {
            myBrowser->GetHost()->WasResized();
        }
    }

    void setZoom(float zoom)
    {
        myZoomLevel = zoom;
        if(myBrowser != NULL) myBrowser->GetHost()->SetZoomLevel(zoom);
    }
    
    bool isFocused() { return myFocused; }
    void setFocusChangedCommand(const String& cmd) { myFocusChangedCommand = cmd; }

private:
    void initialize()
    {
        myInitialized = true;
        CefMainArgs args;
        int result = CefExecuteProcess(args, NULL, NULL);
        if(result >= 0)
        {
            omsg("Exited child CEF3 process");
        }

        CefSettings settings;
        settings.windowless_rendering_enabled = 1;
        settings.single_process = 1;
#ifdef OMEGA_OS_OSX
		String execdir;
		String execname;
		StringUtils::splitFilename(ogetexecpath(), execname, execdir);
		execdir = StringUtils::replaceAll(execdir, "./", "");
		String respath = ostr("%1%Resources", %execdir);
		oflog(Verbose, "[omium::initialize] resources path: %1%", %respath);
        CefString(&settings.resources_dir_path).FromASCII(respath.c_str());
#endif
        //CefString(&settings.browser_subprocess_path).FromASCII("omium_process");
        if(!CefInitialize(args, settings, new BrowserApp(), NULL))
        {
            oerror("CefInitialize failed");
        }
    }

    bool myInitialized;
    CefRefPtr<CefBrowser> myBrowser;
    CefRefPtr<BrowserClient> myBrowserClient;
    CefRefPtr<RenderHandler> myRenderHandler;

    Ref<PixelData> myPixels;
    int myWidth;
    int myHeight;
    float myZoomLevel;
    bool myFocused;
    String myFocusChangedCommand;
};

///////////////////////////////////////////////////////////////////////////////
class OmiumRenderPass : public RenderPass
{
public:
    OmiumRenderPass(Renderer* r, Omium* o) : RenderPass(r, "OmiumRenderPass"),
        myOwner(o)
    {}

    void render(Renderer* client, const DrawContext& context)
    {
    }
private:
    Omium* myOwner;
};

///////////////////////////////////////////////////////////////////////////////
void Omium::initializeRenderer(Renderer* r)
{
    r->addRenderPass(new OmiumRenderPass(r, this));
}

Omium* Omium::instance = NULL;

BOOST_PYTHON_MODULE(omium)
{
    PYAPI_REF_BASE_CLASS(Omium)
        PYAPI_STATIC_REF_GETTER(Omium, getInstance)
        PYAPI_METHOD(Omium, resize)
        PYAPI_REF_GETTER(Omium, getPixels)
        PYAPI_METHOD(Omium, open)
        PYAPI_METHOD(Omium, setZoom)
        PYAPI_METHOD(Omium, setFocusChangedCommand)
        PYAPI_METHOD(Omium, isFocused)
        ;
   
    Omium::instance = new Omium();
}