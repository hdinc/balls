#include <stdio.h>
#include <stdlib.h>
#include <Hd/Shader.h>
#include <Hd/Window.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <android/log.h>
#include <string.h>

struct engine {
    struct android_app* app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    int animating;
};

extern engine engine;

static const char* GetFile(const char* filename);

namespace Hd {

Shader::Shader(const char* vertex, const char* fragment)
{

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vss = GetFile(vertex);
    glShaderSource(vs, 1, &vss, NULL);
    free((void*)vss);
    glCompileShader(vs);
    CheckError(vs, GL_COMPILE_STATUS);

    const char* fss = GetFile(fragment);
    glShaderSource(fs, 1, &fss, NULL);
    free((void*)fss);
    glCompileShader(fs);
    CheckError(fs, GL_COMPILE_STATUS);

    m_ProgramId = glCreateProgram();
    glAttachShader(m_ProgramId, vs);
    glAttachShader(m_ProgramId, fs);
    glLinkProgram(m_ProgramId);
    CheckError(m_ProgramId, GL_LINK_STATUS);
    glValidateProgram(m_ProgramId);
    CheckError(m_ProgramId, GL_VALIDATE_STATUS);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() { glDeleteProgram(m_ProgramId); }

void Shader::CheckError(GLuint id, GLenum pname)
{
    int result;
    if (pname == GL_COMPILE_STATUS) {
        glGetShaderiv(id, pname, &result);
        if (result == GL_FALSE) {
            int length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length);
            glGetShaderInfoLog(id, length, &length, message);
            int type;
            glGetShaderiv(id, GL_SHADER_TYPE, &type);
            __android_log_print(ANDROID_LOG_ERROR, "shader",
                "Failed to complie %s shader ; \n",
                (type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
            __android_log_print(ANDROID_LOG_ERROR, "shader", "%s\n", message);
            glDeleteShader(id);
            exit(-1);
        }
    } else if (pname == GL_LINK_STATUS || pname == GL_VALIDATE_STATUS) {
        glGetProgramiv(id, pname, &result);
        if (result == GL_FALSE) {
            int length;
            glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length);
            glGetProgramInfoLog(id, length, &length, message);
            __android_log_print(ANDROID_LOG_ERROR, "shader",
                "Failed to %s program;\n%s\n",
                pname == GL_LINK_STATUS ? "link" : "validate", message);
            glDeleteProgram(id);
            exit(-1);
        }
    }
}

}

static const char* GetFile(const char* filename)
{

    AAsset* file = AAssetManager_open(
        engine.app->activity->assetManager, filename, AASSET_MODE_BUFFER);

    if (!file) {
        __android_log_print(
            ANDROID_LOG_ERROR, "shader", "could not open asset file: %s", filename);
        exit(1);
    }

    size_t length = AAsset_getLength(file);
    char* buffer = (char*)malloc(length + 1);
    memcpy(buffer, AAsset_getBuffer(file), length);
    buffer[length] = 0;
    AAsset_close(file);
    return buffer;
}
