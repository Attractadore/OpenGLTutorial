#version 330 core
layout (location = 0) in vec2 vPos;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec2 vTex;

out vec3 fColor;
out vec2 fTex;

void main()
{
    fColor = vColor;
    fTex = vTex;
    gl_Position = vec4(vPos, 0.0f, 1.0f);
}