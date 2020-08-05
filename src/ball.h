#ifndef BALL_H
#define BALL_H

#include <Hd/Shader.h>
#include <glm/glm.hpp>

class ball {
public:
    static glm::mat4* projection;
    static Hd::Shader* shader;
    static GLuint vao, vbo;
    static GLint mvp;
    static GLint color;
    static bool drawSpeed;
    static float radius;
    static glm::vec4 c;

    static void initgldata();
    static void destroygldata();

    glm::vec2 speed;
    glm::vec2 loc;
    glm::mat4 model;
    glm::mat4 MVP;

    ball();
    ball(glm::vec2 loc, glm::vec2 speed);

    void draw();
};

#endif // BALL_H
