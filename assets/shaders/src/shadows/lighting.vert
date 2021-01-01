#version 460 core
layout(location = 0) in vec3 vPos;
layout(location = 3) in vec3 vNormal;

layout(location = 0) out VERT_OUT {
    vec3 pos;
    vec3 normal;
}
vOut;

layout(location = 0) uniform mat4 projView;
layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat3 normal;

void main() {
    vec4 worldPos = model * vec4(vPos, 1.0f);
    vOut.pos = worldPos.xyz;
    vOut.normal = normal * vNormal;
    gl_Position = projView * worldPos;
}
