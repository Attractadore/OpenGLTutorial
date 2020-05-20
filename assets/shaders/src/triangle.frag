#version 460 core
layout (location = 0) in vec3 fColor;

layout (location = 0) out vec4 ofColor;

layout (location = 0) uniform float time;

void main()
{
    float scale = (sin(time + length(fColor)) + 1.0f) / 2;
    ofColor = vec4(fColor * scale, 1.0f);
}