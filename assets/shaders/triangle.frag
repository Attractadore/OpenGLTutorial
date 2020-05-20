#version 330 core
in vec3 fColor;

out vec4 ofColor;
uniform float time;

void main()
{
    ofColor = vec4(fColor * (sin(time/2 - exp(length(fColor))) / 2.0 + 0.5), 1.0);
}