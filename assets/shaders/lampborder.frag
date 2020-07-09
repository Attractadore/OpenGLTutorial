#version 330 core
out vec4 fColor;

uniform vec3 lightColor;

void main(){
    fColor = vec4(vec3(1.0f) - lightColor, 1.0f);
}
