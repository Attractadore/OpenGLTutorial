#version 330 core
layout(location = 0) in vec3 vPos;

uniform mat4 transform;
uniform mat4 model;

void main() {
    gl_Position = transform * model * vec4(vPos, 1.0f);
}
