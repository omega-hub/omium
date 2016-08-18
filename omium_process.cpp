#include <cef_app.h>
#include <cef_client.h>
#include <cef_render_handler.h>

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    printf(lpCmdLine);
    CefMainArgs args(hInstance);
    return CefExecuteProcess(args, NULL, NULL);
}
#else
int main(int argc, char *argv[])
{
    CefMainArgs args(argc, argv);
    return CefExecuteProcess(args, NULL, NULL);
}
#endif
