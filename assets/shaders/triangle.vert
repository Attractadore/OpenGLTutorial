#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTex;

out vec3 fPos;
out vec3 fNormal;
out vec2 fTex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normal;

void main()
{
    fPos = vec3(model * vec4(vPos, 1.0f));
    fNormal = normal * vNormal;
    fTex = vTex;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}