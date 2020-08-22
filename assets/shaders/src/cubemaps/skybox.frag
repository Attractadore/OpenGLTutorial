#version 460 core
in vec3 fPos;

out vec4 fColor;

layout(binding = 0) uniform samplerCube cubeMap;

void main() {
    fColor = texture(cubeMap, fPos);
}
