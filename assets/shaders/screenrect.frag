#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D inputFrame;

void main(){
    fColor = texture(inputFrame, fTex);
}
