#version 460 core

layout(triangles, invocations = 4) in;
layout(triangle_strip, max_vertices = 3) out;

layout(binding = 1) restrict readonly buffer CascadePropertiesBuffer {
    mat4 cascadeTransforms[4];
    vec4 cascadeDepths;
    vec4 cascadeSampleSizes;
};

void main() {
    const int i = gl_InvocationID;
    for (int j = 0; j < 3; j++) {
        gl_Layer = i;
        gl_Position = cascadeTransforms[i] * gl_in[j].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
