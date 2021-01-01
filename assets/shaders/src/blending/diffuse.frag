#version 460 core
layout(location = 0) in vec2 tex;

layout(location = 0) out vec4 fColor;

layout(binding = 0) uniform sampler2D diffuse;

void main() {
    fColor = texture(diffuse, tex);
}
