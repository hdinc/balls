#include <Hd.h>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/random.hpp>
#include <cmath>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <initializer_list>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include "ball.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "barzo", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "barzo", __VA_ARGS__))

static int engine_init_display(struct engine* engine);
static void engine_draw_frame(struct engine* engine);
static void engine_term_display(struct engine* engine);
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event);
static void engine_handle_cmd(struct android_app* app, int32_t cmd);

struct engine {
    struct android_app* app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    int animating;
};

struct engine engine;
GLint MVP, COLOR;
GLuint vao_square, vbo_square;
Hd::Shader* shader;
glm::mat4 projection
    = glm::scale(glm::mat4(1), glm::vec3((16.0f / 9.0f) / 17, 1.0 / 17, 1));
float radius = .1f;
const int ballcount = 500;
ball balls[ballcount];

const float borderw = 18;
const float borderh = 32;

static inline double GetAbsoluteTime()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return ((double)tv.tv_usec) / 1000000. + (tv.tv_sec);
}

glm::vec2 rotate90(glm::vec2 x)
{
    glm::vec2 r;

    r.x = -x.y;
    r.y = x.x;

    return r;
}

void borderbounce(ball balls[], unsigned count)
{
    for (unsigned i = 0; i < count; i++) {
        if ((balls[i].loc.x > borderw / 2 - ball::radius && balls[i].speed.x > 0)
            || (balls[i].loc.x < -(borderw / 2 - ball::radius) && balls[i].speed.x < 0)) {
            balls[i].speed.x = -balls[i].speed.x;
        }
        if ((balls[i].loc.y > borderh / 2 - ball::radius && balls[i].speed.y > 0)
            || (balls[i].loc.y < -(borderh / 2 - ball::radius) && balls[i].speed.y < 0)) {
            balls[i].speed.y = -balls[i].speed.y;
        }
    }
}

void ballbounce(ball balls[], unsigned count)
{
    glm::vec2 ballAx, ballAy, ballBx, ballBy, AB, nAB;
    for (unsigned i = 0; i < count - 1; i++) {
        for (unsigned j = i + 1; j < count; j++) {
            if (glm::length(balls[i].loc - balls[j].loc) < 2 * ball::radius) {

                AB = glm::normalize(balls[j].loc - balls[i].loc);
                ballAx = AB * glm::dot(balls[i].speed, AB);
                ballBx = AB * glm::dot(balls[j].speed, AB);
                nAB = rotate90(AB);
                ballAy = nAB * glm::dot(balls[i].speed, nAB);
                ballBy = nAB * glm::dot(balls[j].speed, nAB);

                if (glm::dot(ballAx, ballBx) > 0) {
                    if (glm::dot(AB, ballAx) > 0) {
                        if (glm::length(ballAx) > glm::length(ballBx)) {
                            balls[i].speed = ballBx + ballAy;
                            balls[j].speed = ballAx + ballBy;
                        }
                    } else {
                        if (glm::length(ballBx) > glm::length(ballAx)) {
                            balls[i].speed = ballBx + ballAy;
                            balls[j].speed = ballAx + ballBy;
                        }
                    }
                } else if (glm::dot(ballAx, AB) > 0) {
                    balls[i].speed = ballBx + ballAy;
                    balls[j].speed = ballAx + ballBy;
                }
            }
        }
    }
}

void updateBalls(ball balls[], unsigned count, float deltatime)
{
    for (unsigned i = 0; i < count; i++) {
        balls[i].loc += balls[i].speed * deltatime;
    }
}

void drawBalls(ball balls[], unsigned count)
{

    for (unsigned i = 0; i < count - 1; i++) {
        ball::c = glm::vec4(.4, .4, .4, 1.f);
        balls[i].draw();
    }
    ball::c = glm::vec4(1, .2, .2, 1.f);
    balls[count - 1].draw();
}

float timescale = .001f;
float timei = 0.0f;
float timebefore = 0; // GetAbsoluteTime();
float deltatime = 0;
bool pause = true;
float total_energy;

void android_main(struct android_app* state)
{
    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;
    // engine.animating = 1;

    ball::drawSpeed = false;
    // initballs
    {
        for (unsigned i = 0; i < ballcount; i++) {
            bool ok = false;
            while (!ok) {
            s:
                glm::vec2 l = glm::vec2(
                    glm::linearRand(-borderw / 2.0f + radius, borderw / 2.0f - radius),
                    glm::linearRand(-borderh / 2.0f + radius, borderh / 2.0f - radius));
                for (unsigned j = 0; j < i; j++) {
                    if (glm::length(l - balls[j].loc) <= radius * 2 + 0.1f) {
                        goto s;
                    }
                }
                balls[i].loc = l;
                ok = true;
            }
            balls[i].speed = glm::vec2(
                glm::linearRand(-.001f, .001f), glm::linearRand(-.001f, .001f));
        }
        balls[ballcount - 1].speed = glm::vec2(100.2, -18);
    }

    glClearColor(1, 1, 1, 1);

    while (true) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident = ALooper_pollAll(
                    engine.animating ? 0 : -1, nullptr, &events, (void**)&source))
            >= 0) {

            // Process this event.
            if (source != nullptr) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }
        // timebefore = GetAbsoluteTime();

        if (engine.animating) {
            // Done with events; draw next animation frame.
            engine_draw_frame(&engine);
        }
    }
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine)
{
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint config_attribs[]
        = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8,
              EGL_RED_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_NONE };

    const EGLint context_attribs[]
        = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 2, EGL_NONE };

    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config = nullptr;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, nullptr, nullptr);

    /* Here, the application chooses the configuration it desires.
     * find the best match if possible, otherwise use the very first one
     */

    //  eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    // assert(numConfigs);

    /* Here, the application chooses the configuration it desires.
     * find the best match if possible, otherwise use the very first one
     */
    eglChooseConfig(display, config_attribs, nullptr, 0, &numConfigs);
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    assert(supportedConfigs);
    eglChooseConfig(
        display, config_attribs, supportedConfigs.get(), numConfigs, &numConfigs);
    assert(numConfigs);
    auto i = 0;
    for (; i < numConfigs; i++) {
        auto& cfg = supportedConfigs[i];
        EGLint r, g, b, d;
        if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)
            && eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g)
            && eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)
            && eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) && r == 8 && g == 8
            && b == 8 && d == 0) {

            config = supportedConfigs[i];
            break;
        }
    }
    if (i == numConfigs) {
        config = supportedConfigs[0];
    }

    if (config == nullptr) {
        LOGW("Unable to initialize EGLConfig");
        return -1;
    }

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    surface = eglCreateWindowSurface(display, config, engine->app->window, nullptr);
    context = eglCreateContext(display, config, nullptr, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        LOGW("Unable to create eglcontext");
        return -1;
    }

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    // Check openGL on the system
    auto opengl_info = { GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS };
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("OpenGL Info: %s", info);
    }
    // Initialize GL state.
    // deleted
    //
    shader = new Hd::Shader("shaders/ball.vert", "shaders/ball.frag");
    GLint MVP = glGetUniformLocation(shader->Id(), "MVP");
    GLint COLOR = glGetUniformLocation(shader->Id(), "COLOR");
    shader->Bind();
    glUniform3f(COLOR, 1, 0, 0);
    glm::mat4 mvp(1);
    glUniformMatrix4fv(MVP, 1, GL_FALSE, glm::value_ptr(mvp));

    {
        glGenVertexArrays(1, &vao_square);
        glBindVertexArray(vao_square);
        glGenBuffers(1, &vbo_square);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_square);
        float data[8] = { -borderw / 2, -borderh / 2, -borderw / 2, borderh / 2,
            borderw / 2, borderh / 2, borderw / 2, -borderh / 2 };
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    ball::shader = shader;
    ball::projection = &projection;
    ball::radius = radius;
    ball::initgldata();

    return 0;
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine)
{
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event)
{
    auto* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        pause = 0;
        // timebefore = GetAbsoluteTime();
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd)
{
    auto* engine = (struct engine*)app->userData;
    switch (cmd) {
    case APP_CMD_SAVE_STATE:
        // The system has asked us to save our current state.  Do so.
        // look native activity sample
        break;
    case APP_CMD_INIT_WINDOW:
        // The window is being shown, get it ready.
        if (engine->app->window != nullptr) {
            engine_init_display(engine);
            engine_draw_frame(engine);
        }
        break;
    case APP_CMD_TERM_WINDOW:
        // The window is being hidden or closed, clean it up.
        engine_term_display(engine);
        break;
    case APP_CMD_GAINED_FOCUS:
        engine->animating = 1;
        pause = 0;
        timebefore = GetAbsoluteTime();
        break;
    case APP_CMD_LOST_FOCUS:
        // Also stop animating.
        engine->animating = 0;
        pause = 1;
        engine_draw_frame(engine);
        break;
    default:
        break;
    }
}

static void engine_draw_frame(struct engine* engine)
{
    if (engine->display == nullptr) {
        // No display.
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw
    {
        glUniformMatrix4fv(MVP, 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(vao_square);
        glUniform4f(COLOR, 1, 1, 1, 1);
        glDrawArrays(GL_LINE_LOOP, 0, 4);

        drawBalls(balls, ballcount);
    }

    // Calculate
    {

        if (!pause)
            timei += deltatime = (timescale * (GetAbsoluteTime() - timebefore));

        timebefore = GetAbsoluteTime();

        if (!pause) {
            updateBalls(balls, ballcount, 1 / 60.0f); // deltatime);
            borderbounce(balls, ballcount);
            ballbounce(balls, ballcount);
        }
        total_energy = 0;
        for (unsigned i = 0; i < ballcount; i++) {
            total_energy += (glm::length(balls[i].speed)) * (glm::length(balls[i].speed));
        }

        ball::radius = radius;
    }

    eglSwapBuffers(engine->display, engine->surface);
}
