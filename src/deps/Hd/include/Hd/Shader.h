#pragma once

#include <GLES3/gl3.h>

namespace Hd {

class Shader {

public:
    Shader(const char* vertex, const char* fragment);
    ~Shader();

    void Bind();
    GLuint Id() { return m_ProgramId; }

private:
    void CheckError(GLuint id, GLenum pname);
    GLuint m_ProgramId;
};

inline void Shader::Bind() { glUseProgram(m_ProgramId); }

}
