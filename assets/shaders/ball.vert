#version 300 es

layout(location = 0) in vec2 Pos;
uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(Pos,.5,1);
}
