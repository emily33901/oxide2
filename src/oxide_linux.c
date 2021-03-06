#ifndef _WIN32
#include <string.h>

#include "oxide_internal.h"

#include "GL/glew.h"
#include "GL/glxew.h"

#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

typedef struct _XDisplay Display;
typedef unsigned long    XID;

typedef struct __GLXcontextRec * GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;

static Display *   display;
static int         screen;
static XID         color_map;
static GLXContext  gl_context;
static GLXFBConfig gl_fbconfig;
static bool        mapped = false;
static unsigned    oxide_window_handle;
static unsigned    oxide_target_handle;

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
typedef GLXContext (*glXCreateContextAttribsARBfn)(Display *, GLXFBConfig,
                                                   GLXContext, bool,
                                                   const int *);

bool oxide_init_window() {
    display     = XOpenDisplay(NULL);
    screen      = DefaultScreen(display);
    Window root = DefaultRootWindow(display);

    int dummy;
    if (!glXQueryExtension(display, &dummy, &dummy)) {
        return false;
    }

    static int dblBuf[] = {GLX_RGBA,
                           GLX_RED_SIZE, 1,
                           GLX_GREEN_SIZE, 1,
                           GLX_BLUE_SIZE, 1,
                           GLX_DEPTH_SIZE, 12,
                           GLX_DOUBLEBUFFER,
                           None};

    XVisualInfo *vi = glXChooseVisual(display, screen, dblBuf);

    if (vi == NULL) {
        return false;
    }

    gl_context = glXCreateContext(display, vi, None, 1);
    glXMakeCurrent(display, 0, gl_context);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        return false;
    }

    if (glxewInit() != GLEW_OK) {
        return false;
    }

    GLint attribs[] = {GLX_X_RENDERABLE,
                       GL_TRUE,
                       GLX_DRAWABLE_TYPE,
                       GLX_WINDOW_BIT,
                       GLX_RENDER_TYPE,
                       GLX_RGBA_BIT,
                       GLX_X_VISUAL_TYPE,
                       GLX_TRUE_COLOR,
                       GLX_DEPTH_SIZE,
                       24,
                       GLX_STENCIL_SIZE,
                       8,
                       GLX_RED_SIZE,
                       8,
                       GLX_GREEN_SIZE,
                       8,
                       GLX_BLUE_SIZE,
                       8,
                       GLX_ALPHA_SIZE,
                       8,
                       GLX_DOUBLEBUFFER,
                       GL_TRUE,
                       None};

    int          fbc_count;
    GLXFBConfig *fbc = glXChooseFBConfig(display, screen, attribs, &fbc_count);
    if (fbc == NULL) return false;

    int fbc_best         = -1;
    int fbc_best_samples = -1;
    for (int i = 0; i < fbc_count; ++i) {
        XVisualInfo *info = glXGetVisualFromFBConfig(display, fbc[i]);
        if (info->depth != 32)
            continue;
        int samples;
        glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);
        if (fbc_best < 0 || samples > fbc_best_samples) {
            fbc_best         = i;
            fbc_best_samples = samples;
        }
        XFree(info);
    }
    if (fbc_best == -1) return false;

    gl_fbconfig = fbc[fbc_best];
    XFree(fbc);

    XVisualInfo *info = glXGetVisualFromFBConfig(display, gl_fbconfig);
    if (info == NULL) {
        return false;
    }
    color_map = XCreateColormap(display, root, info->visual, AllocNone);

    XSetWindowAttributes attr;
    attr.background_pixel  = 0x0;
    attr.border_pixel      = 0;
    attr.save_under        = 1;
    attr.override_redirect = 1;
    attr.colormap          = color_map;
    attr.event_mask        = 0x0;
    attr.do_not_propagate_mask =
        (KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
         PointerMotionMask | ButtonMotionMask);

    unsigned long mask = CWBackPixel | CWBorderPixel | CWSaveUnder |
                         CWOverrideRedirect | CWColormap | CWEventMask |
                         CWDontPropagate;

    oxide_window_handle = XCreateWindow(display, root, 0, 0,
                                        800, 600, 0,
                                        info->depth, InputOutput, info->visual, mask, &attr);
    if (oxide_window_handle == 0) return false;

    XShapeCombineMask(display, oxide_window_handle, ShapeInput, 0, 0, None, ShapeSet);
    XShapeSelectInput(display, oxide_window_handle, ShapeNotifyMask);

    XserverRegion region = XFixesCreateRegion(display, NULL, 0);
    XFixesSetWindowShapeRegion(display, oxide_window_handle, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(display, region);

    XFree(info);
    XStoreName(display, oxide_window_handle, "oxide");

    return true;
}

bool oxide_init_os_gl() {
    glXCreateContextAttribsARBfn glXCreateContextAttribsARB = (glXCreateContextAttribsARBfn)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

    int ctx_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
                         GLX_CONTEXT_MINOR_VERSION_ARB, 2, None};

    glXDestroyContext(display, gl_context);

    gl_context = glXCreateContextAttribsARB(display, gl_fbconfig, NULL, GL_TRUE, ctx_attribs);

    if (gl_context == NULL) return false;

    glXMakeCurrent(display, oxide_window_handle, gl_context);

    // Disable the swap interval for all platforms
    // Glew handles ignoring calls for the wrong platform
    // So we can just try all of them
    glXSwapIntervalEXT(display, oxide_window_handle, 0);
    glXSwapIntervalMESA(0);
    glXSwapIntervalSGI(0);

    XSync(display, false);

    // XMapWindow(display, oxide_window_handle);
    // mapped = true;

    return true;
}

bool is_window_child(Display *display, Window target, Window current) {
    Window   retval, root, parent, *children;
    unsigned children_count;

    bool ret = false;

    if (current == target) return true;

    if (XQueryTree(display, current, &root, &parent, &children, &children_count)) {
        for (int i = 0; i < children_count; i++) {
            ret = is_window_child(display, target, children[i]);

            if (ret) break;
        }

        XFree(children);
    }

    return ret;
}

Window window_from_name_search(Display *display, Window display_root, Window current, char const *needle) {
    Window   retval, root, parent, *children;
    unsigned children_count;
    char *   name = NULL;

    // Check if this window has the name we seek
    if (XFetchName(display, current, &name) > 0) {
        int r = strcmp(needle, name);
        XFree(name);
        if (r == 0) return current;
    }

    retval = 0;

    // If it does not: check all subwindows recursively.
    if (0 != XQueryTree(display, current, &root, &parent, &children, &children_count)) {
        unsigned i;
        for (i = 0; i < children_count; ++i) {
            Window win = window_from_name_search(display, display_root, children[i], needle);

            if (win != 0) {
#if 0
                if (current != display_root)
                    retval = current;
                else
#endif
                retval = win;
                break;
            }
        }

        XFree(children);
    }

    return retval;
}

bool oxide_set_target(const char *target_name) {
    Window root         = RootWindow(display, screen);
    oxide_target_handle = window_from_name_search(display, root, root, target_name);

    return oxide_target_handle != 0;
}

Window get_focus_window(Display *d) {
    Window w;
    int    revert_to;
    XGetInputFocus(d, &w, &revert_to);

    return w;
}

Oxide_WindowSize oxide_resize() {
    Oxide_WindowSize new_size = {.width = 0, .height = 0};
    if (oxide_target_handle == 0) return new_size;

    Window root = DefaultRootWindow(display);

    XWindowAttributes window_attribs;
    XGetWindowAttributes(display, oxide_target_handle, &window_attribs);

    int width  = window_attribs.width;
    int height = window_attribs.height;

    Window child;
    int    x = window_attribs.x;
    int    y = window_attribs.y;

    XTranslateCoordinates(display, oxide_target_handle, root, window_attribs.x, window_attribs.y, &x, &y, &child);

    x = x - window_attribs.x;
    y = y - window_attribs.y;

    glViewport(0, 0, width, height);

    XMoveResizeWindow(display, oxide_window_handle, x, y, width, height);

    new_size = (Oxide_WindowSize){.width = width, .height = height};
}

void oxide_begin_os_frame() {
    glXMakeCurrent(display, oxide_window_handle, gl_context);
}

void oxide_end_os_frame() {
    Window focused = get_focus_window(display);

    if (!mapped && is_window_child(display, focused, oxide_target_handle)) {
        XMapWindow(display, oxide_window_handle);
        mapped = true;
    } else if (mapped && !is_window_child(display, focused, oxide_target_handle)) {
        XUnmapWindow(display, oxide_window_handle);
        mapped = false;
    }

    if (mapped) {
        oxide_resize();

        glXSwapBuffers(display, oxide_window_handle);
    }
}

#endif