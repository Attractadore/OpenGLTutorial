#version 460 core
layout(location = 0) in vec3 v3Pos;

layout(location = 0) uniform mat4 m4ProjView;
layout(location = 1) uniform mat4 m4Model;

void main() {
    gl_Position = m4ProjView * m4Model * vec4(v3Pos, 1.0f);
}
