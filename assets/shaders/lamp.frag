#version 330 core
out vec4 fColor;

uniform vec3 lightColor;

void main() {
    fColor = vec4(lightColor, 1.0f);
}
