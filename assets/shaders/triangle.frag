#version 330 core
in vec3 fColor;
in vec2 fTex;

out vec4 ofColor;

uniform float time;
uniform sampler2D backTex, foreTex;

void main()
{
    ofColor = vec4(fColor, 1.0f);
}