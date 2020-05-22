#version 330 core
in vec3 fColor;
in vec2 fTex;

out vec4 ofColor;

uniform sampler2D backTex;
uniform sampler2D foreTex;
uniform float time;

void main()
{
    ofColor = mix(texture(backTex, fTex) * vec4(fColor, 1.0f), texture(foreTex, fTex), (sin(time) + 1) * 0.5f);
}