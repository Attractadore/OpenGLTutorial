#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 4 * 3) out;

layout(binding = 1) restrict readonly buffer CascadePropertiesBuffer {
    mat4 cascadeTransforms[4];
    vec4 cascadeDepths;
    vec4 cascadeSampleSizes;
};

void main() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            gl_Layer = i;
            gl_Position = cascadeTransforms[i] * gl_in[j].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
