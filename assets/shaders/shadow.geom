#version 330 core
#define MAX_LIGHT_TRANSFORMS 60
layout(triangles) in;
layout(triangle_strip, max_vertices = 180) out;

uniform int numLayers;
uniform mat4 lightTransforms[MAX_LIGHT_TRANSFORMS];

void main() {
    for (int i = 0; i < min(numLayers, MAX_LIGHT_TRANSFORMS); i++) {
        for (int j = 0; j < 3; j++) {
            gl_Layer = i;
            gl_Position = lightTransforms[i] * gl_in[j].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}