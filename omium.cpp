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
            if(e.getServiceType() == static_cast<enum Service::ServiceType>(Event::ServiceTypeKeyboard))
            {
                uint sKeyFlags;
                //omsg("Keyboard Event");
                bool isDown = (e.getType() == Event::Down);
                if(e.isFlagSet(Event::Enter))
                    { sendKeyEvent(0x0D,host, isDown); }
                if(e.isFlagSet(Event::ButtonLeft))
                    { sendKeyEvent(0x25,host, isDown); }
                if(e.isFlagSet(Event::ButtonRight))
                    { sendKeyEvent(0x27,host, isDown); }
                if(e.isFlagSet(Event::ButtonDown))
                    { sendKeyEvent(0x28,host, isDown); }
                if(e.isFlagSet(Event::ButtonUp))
                    { sendKeyEvent(0x26,host, isDown); }
                if(e.isFlagSet(Event::Button5))
                    { sendKeyEvent(0x08,host, isDown); }
                if(e.isFlagSet(Event::Button6))
                    { sendKeyEvent(0x09,host, isDown); }
                if(e.isFlagSet(Event::Shift))
                    { sendKeyEvent(0x09,host, isDown); 
                    }
                if(e.isFlagSet(Event::Alt))
                    { sendKeyEvent(0x12,host, isDown); }
                if(e.isFlagSet(Event::Ctrl))
                    { sendKeyEvent(0x11,host, isDown); }
                char c;
                if(e.getChar(&c)) {
                    omsg("Successfully received character in omium:");
                    omsg(string(1,c));
                    sendCharEvent(c, (int)(c), host, e.getFlags());
                }

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
    void sendCharEvent(char character, int code, CefBrowserHost * host, uint flags) {
        omsg("Character Event");
        CefKeyEvent ce;
        ce.windows_key_code = code;
        ce.modifiers = flags;
        ce.character = character;
        ce.type = KEYEVENT_CHAR;
        host->SendKeyEvent(ce);
    }

    void sendKeyEvent(int code, CefBrowserHost * host, bool isDown) {
        omsg("Sending Key Event to CEF, with native key code:");
        omsg(std::to_string(static_cast<int>(code)));
        CefKeyEvent ce;
        ce.windows_key_code = code;
        ce.native_key_code = code;
        //ce.character = 'a';
        //omsg(std::to_string(static_cast<int>(ce.focus_on_editable_field)));
        ce.focus_on_editable_field = true;
        //omsg(std::to_string(static_cast<int>(ce.focus_on_editable_field)));

       // ce.modifiers = 0;
        //ce.is_system_key = 0;
        if (isDown) {
            omsg("event type is DOWN");
            ce.type = KEYEVENT_RAWKEYDOWN;
        } else {
            omsg("event type is UP");
            ce.type = KEYEVENT_KEYUP;
        }
        host->SendKeyEvent(ce);
        //ce.type = KEYEVENT_KEYUP;
        //host->SendKeyEvent(ce);
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