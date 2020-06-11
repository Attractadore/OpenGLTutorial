#version 460 core
layout (location = 0) out vec4 fColor;

layout (location = 3) uniform vec3 lightColor;

void main(){
    fColor = vec4(lightColor, 1.0f);
}