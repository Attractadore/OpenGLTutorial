#version 330 core
out vec4 fColor;

uniform vec3 normalColor;

void main() {
    fColor = vec4(normalColor, 1.0f);
}
