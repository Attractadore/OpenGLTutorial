#version 460 core
layout(location = 0) in vec3 fPos;

layout(location = 0) out vec4 fColor;

layout(binding = 0) uniform samplerCube cubeMap;

void main() {
    fColor = texture(cubeMap, fPos);
}
