#version 330 core
layout(location = 0) in vec3 vPos;

layout(std140) uniform MatrixBlock {
    mat4 projection;
    mat4 view;
}
matrices;

uniform mat4 model;

void main() {
    gl_Position = matrices.projection * matrices.view * model * vec4(vPos, 1.0f);
}
