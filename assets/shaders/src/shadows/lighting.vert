#version 460 core
layout(location = 0) in vec3 vPos;
layout(location = 3) in vec3 vNormal;

layout(location = 0) out VERT_OUT {
    vec3 pos;
    vec3 normal;
}
vOut;

layout(location = 0) uniform mat4 m;
layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat3 normal;

void main() {
    vOut.pos = (model * vec4(vPos, 1.0f)).xyz;
    vOut.normal = normal * vNormal;
    gl_Position = m * vec4(vPos, 1.0f);
}
