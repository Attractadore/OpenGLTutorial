#version 460 core
layout(location = 0) in vec3 v3Pos;

layout(location = 0) uniform mat4 m;

void main() {
    gl_Position = m * vec4(v3Pos, 1.0f);
}
