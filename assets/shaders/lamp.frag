#version 330 core
out vec4 fColor;

uniform vec3 lightColor;

void main() {
    float normFactor = max(lightColor.r, max(lightColor.g, lightColor.b));
    fColor = vec4(lightColor / normFactor, 1.0f);
}
