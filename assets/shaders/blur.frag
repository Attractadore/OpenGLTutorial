#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D inputFrame;
uniform float stride;
uniform int filterWidth;
uniform bool bHorizontal;

void main() {
    vec4 color = vec4(0.0f);
    vec2 strideVec = bHorizontal ? vec2(stride, 0.0f) : vec2(0.0f, stride);
    float totalWeight = 0.0f;
    float theta2 = (filterWidth - 1) / 3.0f;
    theta2 = theta2 * theta2;
    for (int i = -filterWidth + 1; i <= filterWidth - 1; i++) {
        float weight = exp(- i * i / theta2) / sqrt(2 * radians(180.0f) * theta2);
        color += weight * texture(inputFrame, fTex + strideVec * i);
        totalWeight += weight;
    }
    color /= totalWeight;
    fColor = color;
}