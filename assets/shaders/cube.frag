#version 330 core
in vec3 fPos;

out vec4 fColor;

uniform samplerCube cubeMap;

void main(){
    fColor = texture(cubeMap, fPos);
}