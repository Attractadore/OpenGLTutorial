#version 460 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTex;

layout (location = 0) out vec3 fPos;
layout (location = 1) out vec3 fNormal;
layout (location = 2) out vec2 fTex;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;
layout (location = 3) uniform mat3 normal;

void main()
{
    fPos = (model * vec4(vPos, 1.0f)).xyz;
    fNormal = normal * vNormal;
    fTex = vTex;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}