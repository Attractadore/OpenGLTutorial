#version 460

layout (location = 0) in vec3 vPos;

layout (location = 0) out vec3 fPos;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;
layout (location = 3) uniform mat4 local;

void main(){
    fPos = (local * vec4(vPos, 1.0f)).xyz;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}
