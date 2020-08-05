#include <stdio.h>
#include <stdlib.h>
#include <Hd/Window.h>

#include <android_native_app_glue.h>
#include <jni.h>
#include <android/native_activity.h>
#include <GLES3/gl3.h>
#include <android/log.h>

// smelly copied code

struct android_app* gapp;
static int OGLESStarted;
EGLNativeWindowType native_window;
int android_width, android_height;

#define EGL_ZBITS 16

static EGLint const config_attribute_list[]
    = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
          EGL_BUFFER_SIZE, 32, EGL_STENCIL_SIZE, 0, EGL_DEPTH_SIZE, EGL_ZBITS,
// EGL_SAMPLES, 1,
#if ANDROIDVERSION >= 28
          EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#endif
          EGL_NONE };

static const EGLint context_attribute_list[]
    = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

static EGLint window_attribute_list[] = { EGL_NONE };

namespace Hd {

Window::Window(const char* name)
{
    // Do you smell it? That smell, a kind of smelly smell,
    // A smelly smell that smells. Smelly.

    {
        const JNINativeInterface* env = 0;
        const JNINativeInterface** envptr = &env;
        const JNIInvokeInterface** jniiptr
            = (const JNIInvokeInterface**)gapp->activity->vm;
        const JNIInvokeInterface* jnii = *jniiptr;

        env = (*envptr);

        // Get android.app.NativeActivity, then get getWindow method handle,
        // returns view.Window type
        jclass activityClass
            = env->FindClass((JNIEnv*)envptr, "android/app/NativeActivity");
        jmethodID getWindow = env->GetMethodID((JNIEnv*)envptr, activityClass,
            "getWindow", "()Landroid/view/Window;");
        jobject window = env->CallObjectMethod(
            (JNIEnv*)envptr, gapp->activity->clazz, getWindow);

        // Get android.view.Window class, then get getDecorView method handle,
        // returns view.View type
        jclass windowClass
            = env->FindClass((JNIEnv*)envptr, "android/view/Window");
        jmethodID getDecorView = env->GetMethodID((JNIEnv*)envptr, windowClass,
            "getDecorView", "()Landroid/view/View;");
        jobject decorView
            = env->CallObjectMethod((JNIEnv*)envptr, window, getDecorView);

        // Get the flag values associated with systemuivisibility
        jclass viewClass = env->FindClass((JNIEnv*)envptr, "android/view/View");
        const int flagLayoutHideNavigation
            = env->GetStaticIntField((JNIEnv*)envptr, viewClass,
                env->GetStaticFieldID((JNIEnv*)envptr, viewClass,
                    "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION", "I"));
        const int flagLayoutFullscreen
            = env->GetStaticIntField((JNIEnv*)envptr, viewClass,
                env->GetStaticFieldID((JNIEnv*)envptr, viewClass,
                    "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN", "I"));
        const int flagLowProfile
            = env->GetStaticIntField((JNIEnv*)envptr, viewClass,
                env->GetStaticFieldID((JNIEnv*)envptr, viewClass,
                    "SYSTEM_UI_FLAG_LOW_PROFILE", "I"));
        const int flagHideNavigation
            = env->GetStaticIntField((JNIEnv*)envptr, viewClass,
                env->GetStaticFieldID((JNIEnv*)envptr, viewClass,
                    "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I"));
        const int flagFullscreen
            = env->GetStaticIntField((JNIEnv*)envptr, viewClass,
                env->GetStaticFieldID((JNIEnv*)envptr, viewClass,
                    "SYSTEM_UI_FLAG_FULLSCREEN", "I"));
        const int flagImmersiveSticky
            = env->GetStaticIntField((JNIEnv*)envptr, viewClass,
                env->GetStaticFieldID((JNIEnv*)envptr, viewClass,
                    "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I"));

        jmethodID setSystemUiVisibility = env->GetMethodID(
            (JNIEnv*)envptr, viewClass, "setSystemUiVisibility", "(I)V");

        // Call the decorView.setSystemUiVisibility(FLAGS)
        env->CallVoidMethod((JNIEnv*)envptr, decorView, setSystemUiVisibility,
            (flagLayoutHideNavigation | flagLayoutFullscreen | flagLowProfile
                | flagHideNavigation | flagFullscreen | flagImmersiveSticky));

        // now set some more flags associated with layoutmanager -- note the $
        // in the class path search for api-versions.xml
        // https://android.googlesource.com/platform/development/+/refs/tags/android-9.0.0_r48/sdk/api-versions.xml

        jclass layoutManagerClass = env->FindClass(
            (JNIEnv*)envptr, "android/view/WindowManager$LayoutParams");
        const int flag_WinMan_Fullscreen
            = env->GetStaticIntField((JNIEnv*)envptr, layoutManagerClass,
                (env->GetStaticFieldID((JNIEnv*)envptr, layoutManagerClass,
                    "FLAG_FULLSCREEN", "I")));
        const int flag_WinMan_KeepScreenOn
            = env->GetStaticIntField((JNIEnv*)envptr, layoutManagerClass,
                (env->GetStaticFieldID((JNIEnv*)envptr, layoutManagerClass,
                    "FLAG_KEEP_SCREEN_ON", "I")));
        const int flag_WinMan_hw_acc
            = env->GetStaticIntField((JNIEnv*)envptr, layoutManagerClass,
                (env->GetStaticFieldID((JNIEnv*)envptr, layoutManagerClass,
                    "FLAG_HARDWARE_ACCELERATED", "I")));
        //    const int flag_WinMan_flag_not_fullscreen =
        //    env->GetStaticIntField(layoutManagerClass,
        //    (env->GetStaticFieldID(layoutManagerClass,
        //    "FLAG_FORCE_NOT_FULLSCREEN", "I") ));
        // call window.addFlags(FLAGS)
        env->CallVoidMethod((JNIEnv*)envptr, window,
            (env->GetMethodID(
                (JNIEnv*)envptr, windowClass, "addFlags", "(I)V")),
            (flag_WinMan_Fullscreen | flag_WinMan_KeepScreenOn
                | flag_WinMan_hw_acc));

        jnii->DetachCurrentThread((JavaVM*)jniiptr);
    }

    {
        int ret;
        EGLint egl_major, egl_minor;
        EGLConfig config;
        EGLint num_config;
        EGLContext context;

        // This MUST be called before doing any initialization.
        int events;
        while (!OGLESStarted) {
            struct android_poll_source* source;
            if (ALooper_pollAll(0, 0, &events, (void**)&source) >= 0) {
                if (source != NULL)
                    source->process(gapp, source);
            }
        }

        egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (egl_display == EGL_NO_DISPLAY) {
            __android_log_print(
                ANDROID_LOG_ERROR, "window:", "Error: No display found!\n");
            exit(1);
        }

        if (!eglInitialize(egl_display, &egl_major, &egl_minor)) {
            __android_log_print(
                ANDROID_LOG_ERROR, "window:", "Error: eglInitialise failed!\n");
            exit(1);
        }
        __android_log_print(ANDROID_LOG_ERROR,
            "window:", "EGL Version: \"%s\"\n",
            eglQueryString(egl_display, EGL_VERSION));
        __android_log_print(ANDROID_LOG_ERROR, "window", "EGL Vendor: \"%s\"\n",
            eglQueryString(egl_display, EGL_VENDOR));
        __android_log_print(ANDROID_LOG_ERROR, "window",
            "EGL Extensions: \"%s\"\n",
            eglQueryString(egl_display, EGL_EXTENSIONS));

        eglChooseConfig(
            egl_display, config_attribute_list, &config, 1, &num_config);
        __android_log_print(
            ANDROID_LOG_ERROR, "window", "Config: %d\n", num_config);

        __android_log_print(ANDROID_LOG_ERROR, "window", "Creating Context\n");
        context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT,
            //				NULL );
            context_attribute_list);
        if (context == EGL_NO_CONTEXT) {
            __android_log_print(ANDROID_LOG_ERROR, "window",
                "Error: eglCreateContext failed: 0x%08X\n", eglGetError());
            exit(1);
        }
        __android_log_print(
            ANDROID_LOG_ERROR, "window", "Context Created %p\n", context);

        if (native_window && !gapp->window) {
            __android_log_print(ANDROID_LOG_ERROR, "window",
                "WARNING: App restarted without a window.  Cannot progress.\n");
            exit(1);
        }

        __android_log_print(ANDROID_LOG_ERROR, "window", "Getting Surface %p\n",
            native_window = gapp->window);

        if (!native_window) {
            __android_log_print(
                ANDROID_LOG_ERROR, "window", "FAULT: Cannot get window\n");
            exit(1);
        }
        android_width = ANativeWindow_getWidth(native_window);
        android_height = ANativeWindow_getHeight(native_window);
        __android_log_print(ANDROID_LOG_ERROR, "window",
            "Width/Height: %dx%d\n", android_width, android_height);
        egl_surface = eglCreateWindowSurface(
            egl_display, config, gapp->window, window_attribute_list);
        __android_log_print(
            ANDROID_LOG_ERROR, "window", "Got Surface: %p\n", egl_surface);

        if (egl_surface == EGL_NO_SURFACE) {
            __android_log_print(ANDROID_LOG_ERROR, "window",
                "Error: eglCreateWindowSurface failed: "
                "0x%08X\n",
                eglGetError());
            exit(1);
        }

        if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, context)) {
            __android_log_print(ANDROID_LOG_ERROR, "window",
                "Error: eglMakeCurrent() failed: 0x%08X\n", eglGetError());
            exit(1);
        }

        __android_log_print(ANDROID_LOG_ERROR, "window", "GL Vendor: \"%s\"\n",
            glGetString(GL_VENDOR));
        __android_log_print(ANDROID_LOG_ERROR, "window",
            "GL Renderer: \"%s\"\n", glGetString(GL_RENDERER));
        __android_log_print(ANDROID_LOG_ERROR, "window", "GL Version: \"%s\"\n",
            glGetString(GL_VERSION));
        __android_log_print(ANDROID_LOG_ERROR, "window",
            "GL Extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));
    }
}

void Window::HandleInput()
{

    int events;
    struct android_poll_source* source;
    while (ALooper_pollAll(0, 0, &events, (void**)&source) >= 0) {
        if (source != NULL) {
            source->process(gapp, source);
        }
    }
}
}
