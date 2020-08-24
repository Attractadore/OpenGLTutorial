#version 460 core

layout(location = 0) out vec4 fColor;

layout(location = 3) uniform vec3 sColor;

void main() {
    fColor = vec4(sColor, 1.0f);
}
