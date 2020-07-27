#version 330 core
layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 startPos;
layout(location = 3) in vec3 fallVec;

out VERT_OUT {
    vec3 pos;
    vec3 normal;
    vec2 tex;
}
vOut;

layout(std140) uniform MatrixBlock {
    mat4 projection;
    mat4 view;
}
matrices;

uniform mat4 model;
uniform float freq;
uniform float time;

void main() {
    float phase = gl_InstanceID;
    vec4 worldPos = model * vec4(vPos, 1.0f);
    worldPos.xyz = worldPos.xyz + startPos + fallVec * sin(freq * time + phase);
    vOut.pos = worldPos.xyz;
    vOut.normal = vNormal;
    vOut.tex = vec2(0.0f);
    gl_Position = matrices.projection * matrices.view * worldPos;
}
