#version 460 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec4 vColor;

layout (location = 0) out vec4 ovColor;

void main()
{
    gl_Position = vec4(vPos, 1.0);
    ovColor = vColor;
}