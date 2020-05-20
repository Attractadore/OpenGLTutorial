#version 460 core
layout (location = 0) in vec2 vPos;
layout (location = 1) in vec3 vColor;

layout (location = 0) out vec3 ovColor;

layout (location = 0) uniform float time;

void main()
{
    mat2 rotMat = mat2(
         cos(time), sin(time),
        -sin(time), cos(time)
    );
    gl_Position = vec4(rotMat * normalize(vPos) * 0.75f, 0.0f, 1.0f);
    ovColor = vColor;
}