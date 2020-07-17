#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D inputFrame;

vec3 toGrey(vec3 color) {
    float gs = 0.2126f * color.r + 0.7152f * color.g + 0.0722 * color.b;
    return vec3(gs);
}

void main() {
    vec3 color = texture(inputFrame, fTex).rgb;
    vec3 gs = toGrey(color);
    color = mix(color, gs, 0.8f);
    fColor = vec4(color, 1.0f);
}
