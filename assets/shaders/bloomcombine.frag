#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D baseFrame, bloomFrame;

void main() {
    vec3 baseColor = texture(baseFrame, fTex).rgb;
    vec3 bloomColor = texture(bloomFrame, fTex).rgb;
    fColor = vec4(baseColor + bloomColor, 1.0f);
}
