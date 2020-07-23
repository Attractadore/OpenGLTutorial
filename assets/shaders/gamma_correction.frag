#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D inputFrame;
uniform float correctionFactor;

void main() {
    vec3 color = texture(inputFrame, fTex).rgb;
    fColor = vec4(pow(color, vec3(correctionFactor)), 1.0f);
}
