#version 460 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec2 vTex;

layout (location = 0) out vec3 ovColor;
layout (location = 1) out vec2 ovTex;

layout (location = 3) uniform mat4 model;
layout (location = 4) uniform mat4 view;
layout (location = 5) uniform mat4 projection;

void main()
{
    ovColor = vColor;
    ovTex = vTex;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}