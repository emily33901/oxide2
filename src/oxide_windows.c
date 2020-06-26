#ifdef _WIN32

#include "oxide_internal.h"

#define GLCOMPAT

#define GLEW_STATIC
#include "GL/glew.h"

#include "GL/wglew.h"

#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#include <gl/gl.h>

typedef void *Handle;

Handle oxide_window_handle;
Handle oxide_target_handle;

Handle oxide_dc;
Handle oxide_rc;

static const char oxide_window_name[] = "oxide";

bool oxide_init_os_gl() {
    GLuint PixelFormat;

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 32;
    pfd.cDepthBits   = 24;
    pfd.cAlphaBits   = 8;
    pfd.cStencilBits = 8;
    pfd.iLayerType   = PFD_MAIN_PLANE;

    oxide_dc    = GetDC((HWND)oxide_window_handle);
    PixelFormat = ChoosePixelFormat(oxide_dc, &pfd);
    SetPixelFormat(oxide_dc, PixelFormat, &pfd);
    oxide_rc = wglCreateContext(oxide_dc);
    wglMakeCurrent(oxide_dc, oxide_rc);

    // calling glewinit NOW because the inside glew, there is mistake to fix...
    // This is the joy of using Core. The query glGetString(GL_EXTENSIONS) is deprecated from the Core profile.
    // You need to use glGetStringi(GL_EXTENSIONS, <index>) instead. Sounds like a "bug" in GLEW.

    if (!wglCreateContextAttribsARB) wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (!wglSwapIntervalEXT) wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglCreateContextAttribsARB) {
        HGLRC atrib_rc = NULL;
        int   attribList[] =
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                WGL_CONTEXT_MINOR_VERSION_ARB, 2,
#ifdef GLCOMPAT
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#else
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
                WGL_CONTEXT_FLAGS_ARB,
        //WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB|
#ifndef GLCOMPAT
                WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |
#endif
#ifdef _DEBUG
                    WGL_CONTEXT_DEBUG_BIT_ARB
#else
                0
#endif
                ,
                0, 0};
        if (!(atrib_rc = wglCreateContextAttribsARB((HDC)oxide_dc, 0, attribList))) {
            return false;
        }
        if (!wglMakeCurrent(oxide_dc, atrib_rc)) {
            return false;
        } else {
            wglDeleteContext(oxide_rc);
            oxide_rc = atrib_rc;
#ifdef _DEBUG
            if (!glDebugMessageCallbackARB) {
                glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)wglGetProcAddress("glDebugMessageCallbackARB");
                glDebugMessageControlARB  = (PFNGLDEBUGMESSAGECONTROLARBPROC)wglGetProcAddress("glDebugMessageControlARB");
            }
            if (glDebugMessageCallbackARB) {
                //glDebugMessageCallbackARB(glErrorCallback, NULL);
                glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH_ARB, 0, NULL, GL_TRUE);
            }
#endif
            wglSwapIntervalEXT(0);
        }
    }

    return true;
}

void *__stdcall oxide_window_proc(HWND window_handle, unsigned message, size_t w_param, size_t l_param) {
    switch (message) {
    case WM_CREATE: {
        break;
    }
    case WM_CLOSE: {
        DestroyWindow(oxide_window_handle);
        break;
    }
    //release resources
    case WM_DESTROY: {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(oxide_rc);
        DeleteDC(oxide_dc);
        ReleaseDC(oxide_window_handle, oxide_dc);
        PostQuitMessage(0);
        UnregisterClass(oxide_window_name, GetModuleHandle(NULL));
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(oxide_window_handle, &ps);
        EndPaint(oxide_window_handle, &ps);
        return NULL;
    }
    }

    return (void *)DefWindowProc(window_handle, message, w_param, l_param);
}

bool oxide_init_window() {
    Handle local_instance = GetModuleHandle(NULL);

    WNDCLASSEX winClass;
    MSG        uMsg;

    memset(&uMsg, 0, sizeof(uMsg));

    winClass.lpszClassName = oxide_window_name;
    winClass.cbSize        = sizeof(WNDCLASSEX);
    winClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    winClass.lpfnWndProc   = (WNDPROC)&oxide_window_proc;
    winClass.hInstance     = local_instance,
    winClass.hIcon         = 0x0;
    winClass.hIconSm       = 0x0;
    winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    winClass.hbrBackground = (HBRUSH)CreateSolidBrush(0x00000000);
    winClass.lpszMenuName  = NULL;
    winClass.cbClsExtra    = 0;
    winClass.cbWndExtra    = 0;

    if (!RegisterClassEx(&winClass))
        return false;

    oxide_window_handle = CreateWindowExA(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST,
                                          oxide_window_name, oxide_window_name,
                                          WS_VISIBLE | WS_POPUP | WS_POPUP,
                                          0,
                                          0,
                                          0,
                                          0,
                                          NULL,
                                          NULL,
                                          local_instance,
                                          NULL);
    if (oxide_window_handle == NULL) {
        int err = GetLastError();
        return false;
    }

    SetLayeredWindowAttributes(oxide_window_handle, 0, 255, LWA_ALPHA);

    DWM_BLURBEHIND bb   = {0};
    HRGN           hRgn = CreateRectRgn(0, 0, -1, -1);
    bb.dwFlags          = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.hRgnBlur         = hRgn;
    bb.fEnable          = TRUE;
    DwmEnableBlurBehindWindow(oxide_window_handle, &bb);

    //ShowWindow(window_handle, 10);
    //UpdateWindow(window_handle);

    return true;
}

bool oxide_set_target(const char *target_name) {
    // Try and find by class name first
    oxide_target_handle = FindWindowA(target_name, NULL);
    if (oxide_target_handle != NULL) return true;

    // Then try and find by window name
    oxide_target_handle = FindWindowA(NULL, target_name);
    if (oxide_target_handle == NULL) return false;

    return true;
}

Oxide_WindowSize oxide_resize() {
    Oxide_WindowSize new_size = {.width = 0, .height = 0};
    if (oxide_target_handle == NULL || oxide_window_handle == NULL) return new_size; // we cant size the window yet

    RECT window_bounds;
    RECT client_bounds;

    // get the inner and outer bounds of the target
    GetWindowRect(oxide_target_handle, &window_bounds);
    GetClientRect(oxide_target_handle, &client_bounds);

#define rect_size_zero(a) (((a.right - a.left) == 0 || (a.bottom - a.top) == 0))
    if (rect_size_zero(window_bounds) || rect_size_zero(client_bounds)) return new_size;

#undef rect_size_zero

    //width and height of client rect
    int width  = client_bounds.right - client_bounds.left;
    int height = client_bounds.bottom - client_bounds.top;

    int posx = window_bounds.right - width;
    int posy = window_bounds.bottom - height;

    SetWindowPos(oxide_window_handle, 0, posx, posy, width, height, 0);

    glViewport(0, 0, width, height);
    glOrtho(client_bounds.left, client_bounds.right, client_bounds.bottom, client_bounds.top, 0, 1);

    new_size = (Oxide_WindowSize){.width = width, .height = height};
    return new_size;
}

void oxide_begin_os_frame() {
    wglMakeCurrent(oxide_dc, oxide_rc);
}

void oxide_end_os_frame() {
    wglMakeCurrent(oxide_dc, oxide_rc);

    Handle active_window = GetForegroundWindow();

    // dont paint if the target is minimised
    if (!IsIconic(oxide_target_handle) && active_window == oxide_target_handle) {
        ShowWindow(oxide_window_handle, SW_SHOWNORMAL);
        SwapBuffers(oxide_dc);

        oxide_resize();
    } else {
        ShowWindow(oxide_window_handle, SW_HIDE);
    }

    // process any new messages
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return;
}
#endif
