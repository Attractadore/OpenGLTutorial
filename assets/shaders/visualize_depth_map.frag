#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2DArray inputFrame;

void main() {
    vec3 color = vec3(texture(inputFrame, vec3(fTex, 0.0f)).r);
    fColor = vec4(color, 1.0f);
}
