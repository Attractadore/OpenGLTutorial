#version 460 core
layout (location = 0) in vec2 vPos;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec2 vTex;

layout (location = 0) out vec3 ovColor;
layout (location = 1) out vec2 ovTex;

void main()
{
    ovColor = vColor;
    ovTex = vTex;
    gl_Position = vec4(vPos, 0.0f, 1.0f);
}