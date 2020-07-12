#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;

out VERT_OUT {
    vec4 normalRoot;
    vec4 normalEnd;
} vOut;

layout (std140) uniform MatrixBlock {
    mat4 projection;
    mat4 view;
} matrices;

uniform mat4 model;
uniform float normalScale;

void main()
{
    vOut.normalRoot = matrices.projection * matrices.view * model * vec4(vPos, 1.0f);
    vOut.normalEnd  = matrices.projection * matrices.view * model * vec4(vPos + normalScale * normalize(vNormal), 1.0f);
    gl_Position = vOut.normalRoot;
}