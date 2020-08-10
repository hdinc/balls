#include "ball.h"
#include <glm/gtc/type_ptr.hpp>

glm::mat4* ball::projection = 0;
Hd::Shader* ball::shader = 0;
GLuint ball::vao = 0;
GLuint ball::vbo = 0;
GLint ball::mvp = 0;
GLint ball::color = 0;
bool ball::drawSpeed = true;
float ball::radius = 1.f;
glm::vec4 ball::c = glm::vec4(.4, .4, .4, 1.f);

ball::ball()
    : loc(0.f, 0.f)
    , speed(0.f, 0.f)
    , model(1.f)
    , MVP(1.f)
{
}

ball::ball(glm::vec2 loc, glm::vec2 speed)
    : loc(loc)
    , speed(speed)
    , model(1.f)
    , MVP(1.f)
{
}

void ball::draw()
{
    shader->Bind();
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    model = glm::translate(glm::mat4(1), glm::vec3(loc.x, loc.y, 0));
    model = glm::scale(model, glm::vec3(radius, radius, 1.f));
    MVP = *projection * model;
    glUniformMatrix4fv(mvp, 1, GL_FALSE, glm::value_ptr(MVP));
    glBufferSubData(
        GL_ARRAY_BUFFER, 206 * sizeof(float), 2 * sizeof(float), glm::value_ptr(speed));
    glUniform4f(color, c.x, c.y, c.z, c.w);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 102);
    glUniform4f(color, 0, 0, 1, 1.f);
    if (drawSpeed)
        glDrawArrays(GL_LINES, 102, 2);
}

void ball::initgldata()
{
    mvp = glGetUniformLocation(shader->Id(), "MVP");
    color = glGetUniformLocation(shader->Id(), "COLOR");
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    float circle[104][2];
    for (int i = 1; i < 101; i += 1) {
        circle[i][0] = sin((i - 1) * 2 * M_PI / 100);
        circle[i][1] = cos((i - 1) * 2 * M_PI / 100);
    }
    circle[0][0] = 0;
    circle[0][1] = 0;
    circle[101][0] = 0;
    circle[101][1] = 1;
    circle[102][0] = 0;
    circle[102][1] = 0;

    glBufferData(GL_ARRAY_BUFFER, sizeof(circle), circle, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void ball::destroygldata()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}
