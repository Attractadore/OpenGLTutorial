#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTex;

out VERT_OUT {
    vec3 pos;
    vec3 normal;
    vec2 tex;
} vOut;

layout (std140) uniform MatrixBlock {
    mat4 projection;
    mat4 view;
} matrices;

uniform mat4 model;
uniform mat3 normal;

void main()
{
    vOut.pos = vec3(model * vec4(vPos, 1.0f));
    vOut.normal = normal * vNormal;
    vOut.tex = vTex;
    gl_Position = matrices.projection * matrices.view * model * vec4(vPos, 1.0f);
}