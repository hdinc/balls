#pragma once

#include <EGL/egl.h>
#include <android_native_app_glue.h>

extern struct android_app* gapp;

namespace Hd {

class Window {

public:
    Window(const char* name);
    void HandleInput();
    void SwapBuffers();

private:
    EGLDisplay egl_display;
    EGLSurface egl_surface;
};

inline void Window::SwapBuffers() { eglSwapBuffers(egl_display, egl_surface); }

}
