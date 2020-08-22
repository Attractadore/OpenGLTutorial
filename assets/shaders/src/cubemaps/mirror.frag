#version 460 core

in VERT_OUT {
    vec3 pos;
    vec3 normal;
}
fIn;

out vec4 fColor;

layout(location = 4) uniform vec3 cameraPos;
layout(binding = 0) uniform samplerCube cubeMap;

void main() {
    vec3 fragPos = fIn.pos;
    vec3 fragNormal = normalize(fIn.normal);
    vec3 cameraDir = normalize(cameraPos - fragPos);
    vec3 reflectDir = reflect(-cameraDir, fragNormal);
    fColor = texture(cubeMap, reflectDir);
}
