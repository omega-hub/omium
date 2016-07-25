#include <omega.h>
#include <omega/PythonInterpreterWrapper.h>

#include <cef_app.h>
#include <cef_client.h>
#include <cef_render_handler.h>

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
        myZoomLevel(1)
    {
        ModuleServices::addModule(this);
        setPriority(EngineModule::PriorityHigh);
        resize(600, 400);
        myOffscreen = false;
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
            myBrowser = CefBrowserHost::CreateBrowserSync(window_info, myBrowserClient.get(), url, browserSettings, nullptr);
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
            if(e.getServiceType() == static_cast<enum Service::ServiceType>(Event::ServiceTypeKeyboard) && 
                e.getType() == Event::Down)
            {
                omsg("Keyboard Event");
                // CefKeyEvent ce;
                // Process special keys
                if(e.isFlagSet(Event::Enter))
                {
                    //ce.windows_key_code = 0x0D;
                    //omsg("Enter Event");
                    sendKeyEvent(0x0D,host);
                }
                if(e.isFlagSet(Event::ButtonLeft))
                {
                    // ce.windows_key_code = 0x25;
                    // omsg("Left Key Event");
                    sendKeyEvent(0x25,host);
                }
                if(e.isFlagSet(Event::ButtonRight))
                {
                    // ce.windows_key_code = 0x27;
                    // omsg("Right Key Event");
                    sendKeyEvent(0x27,host);
                }
                if(e.isFlagSet(Event::ButtonDown))
                {
                    // ce.windows_key_code = 0x28;
                    // omsg("Down Key Event");
                    sendKeyEvent(0x28,host);
                }
                if(e.isFlagSet(Event::ButtonUp))
                {
                    // ce.windows_key_code = 0x26;
                    // omsg("Up Key Event");
                    sendKeyEvent(0x26,host);
                }
                if(e.isFlagSet(Event::Button5))
                {
                    // ce.windows_key_code = 0x08;
                    // omsg("Backspace Key Event");
                    sendKeyEvent(0x08,host);
                }
                if(e.isFlagSet(Event::Button6))
                {
                    // ce.windows_key_code = 0x09;
                    // omsg("Tab Key Event");
                    sendKeyEvent(0x09,host);
                }
                if(e.isFlagSet(Event::Shift))
                {
                    // ce.windows_key_code = 0x10;
                    // omsg("Shift Key Event");
                    sendKeyEvent(0x09,host);
                }
                if(e.isFlagSet(Event::Alt))
                {
                    // ce.windows_key_code = 0x12;
                    // omsg("Alt Key Event");
                    sendKeyEvent(0x12,host);
                }
                if(e.isFlagSet(Event::Ctrl))
                {
                    // ce.windows_key_code = 0x11;
                    // omsg("Control Key Event");
                    sendKeyEvent(0x11,host);
                }

                char c;
                if(e.getChar(&c))
                {
                    // ce.character = c;
                    // omsg("Character Key Event");
                    // omsg(string(1,c));
                    // ce.windows_key_code = (int)(c);
                    sendKeyEvent((int)(c),host);
                }

                // ce.type = KEYEVENT_KEYDOWN;
                // host->SendKeyEvent(ce);

            } else if(e.getServiceType() == Service::Pointer)
            {
                CefMouseEvent me;
                me.x = e.getPosition()[0];
                me.y = e.getPosition()[1];
                if(e.getType() == Event::Move)
                {
                    host->SendMouseMoveEvent(me, false);
                }
                else if(e.getType() == Event::Down || e.getType() == Event::Up)
                {
                    // If we click on something opaque, the browser intercepts
                    // this mouse event
                    myPixels->beginPixelAccess();
                    if(myPixels->getPixelA(me.x, me.y) > 0) e.setProcessed();
                    myPixels->endPixelAccess();

                    CefBrowserHost::MouseButtonType mbt = MBT_LEFT;
                    if(e.isFlagSet(Event::Right)) mbt = MBT_RIGHT;
                    else if(e.isFlagSet(Event::Middle)) mbt = MBT_MIDDLE;

                    host->SendMouseClickEvent(me, mbt, e.getType() == Event::Up, 1);
                }
            }
        }
    }

    PixelData* getPixels() { return myPixels; }

    void sendKeyEvent(int code, CefBrowserHost * host) {
        omsg("Sending Key Event");
        CefKeyEvent ce;
        ce.windows_key_code = code;
        ce.type = KEYEVENT_KEYDOWN;
        host->SendKeyEvent(ce);
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

    void setOffscreen(bool value)
    {
        myOffscreen = value;
    }
    bool isOffscreen()
    {
        return myOffscreen;
    }
    void setZoom(float zoom)
    {
        myZoomLevel = zoom;
        if(myBrowser != NULL) myBrowser->GetHost()->SetZoomLevel(zoom);
    }

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
        //settings.single_process = 1;
        CefString(&settings.browser_subprocess_path).FromASCII("omium_process");
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
    bool myOffscreen;
    float myZoomLevel;
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
        PYAPI_METHOD(Omium, setOffscreen)
        PYAPI_METHOD(Omium, isOffscreen)
        PYAPI_METHOD(Omium, open)
        PYAPI_METHOD(Omium, setZoom)
        ;
   
    Omium::instance = new Omium();
}