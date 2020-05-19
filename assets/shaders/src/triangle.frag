#version 460 core
layout (location = 0) in vec4 fColor;

layout (location = 0) out vec4 ofColor;

void main()
{
    ofColor = fColor;
}