#version 460 core
layout(location = 0) in vec3 vPos;
layout(location = 3) in vec3 vNormal;

layout(location = 0) out VERT_OUT {
    vec3 pos;
    vec3 normal;
}
vOut;

layout(location = 0) uniform mat4 projection;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 model;
layout(location = 3) uniform mat3 normal;

void main() {
    vOut.pos = vec3(model * vec4(vPos, 1.0f));
    vOut.normal = normal * vNormal;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}
