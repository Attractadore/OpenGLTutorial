#version 460 core

const int MAX_CASCADES = 4;

layout(triangles) in;
layout(triangle_strip, max_vertices = MAX_CASCADES * 3) out;

layout(location = 10) uniform int numCascades;
layout(location = 11) uniform mat4 cascadeTransforms[MAX_CASCADES];

void main() {
    int numIter = min(numCascades, MAX_CASCADES);
    for (int i = 0; i < numIter; i++) {
        for (int j = 0; j < 3; j++) {
            gl_Layer = i;
            gl_Position = cascadeTransforms[i] * gl_in[j].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
