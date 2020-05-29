#version 460 core
layout (location = 0) in vec3 fColor;
layout (location = 1) in vec2 fTex;

layout (location = 0) out vec4 ofColor;

layout (location = 0) uniform float time;
layout (location = 1) uniform sampler2D backTex;
layout (location = 2) uniform sampler2D foreTex;

void main()
{
    ofColor = vec4(fColor, 1.0f);
}