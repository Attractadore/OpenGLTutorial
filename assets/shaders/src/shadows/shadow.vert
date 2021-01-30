#version 460 core

#include "indices.glsl"

layout(location = 0) in vec3 v3Pos;

layout(location = SHADOW_VERT_TRANSFORM_LOCATION)
    uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(v3Pos, 1.0f);
}
