#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D inputFrame;
uniform float intencity;

void main(){
    vec3 color = pow(texture(inputFrame, fTex).rgb, vec3(intencity));
    fColor = vec4(color, 1.0f);
}