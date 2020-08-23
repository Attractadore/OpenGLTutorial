#version 460 core
layout(location = 0) in vec3 vPos;
layout(location = 4) in vec2 vTex;

layout(location = 0) out vec2 tex;

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 model;

void main() {
    tex = vTex;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}