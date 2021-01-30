#version 460 core

#include "indices.glsl"

layout(location = 0) in vec3 vPos;
layout(location = 3) in vec3 vNormal;

layout(location = 0) out vec3 pos_v;
layout(location = 1) out vec3 normal_v;

layout(location = LIGHTING_VERT_PROJ_VIEW_MODEL_LOCATION)
    uniform mat4 proj_view_model_mat;
layout(location = LIGHTING_VERT_MODEL_LOCATION)
    uniform mat4 model_mat;
layout(location = LIGHTING_VERT_NORMAL_LOCATION)
    uniform mat3 normal_mat;

void main() {
    pos_v = (model_mat * vec4(vPos, 1.0f)).xyz;
    normal_v = normal_mat * vNormal;
    gl_Position = proj_view_model_mat * vec4(vPos, 1.0f);
}
