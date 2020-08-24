#version 460 core
layout(location = 0) in vec3 vPos;
layout(location = 5) in vec3 iPos;

layout(location = 0) uniform mat4 projview;

void main() {
    gl_Position = projview * vec4(vPos + iPos, 1.0f);
}
