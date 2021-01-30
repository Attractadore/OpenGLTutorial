#ifndef CASCADE_SSBO_GLSL
#define CASCADE_SSBO_GLSL

#ifndef CASCADE_SSBO_QUALIFIERS
#error
#endif

#include "indices.glsl"

layout(binding = CASCADE_SSBO_BINDING, std430)
    CASCADE_SSBO_QUALIFIERS buffer CascadePropertiesBuffer {
    mat4 cascadeTransforms[4];
    vec4 cascadeDepths;
    vec4 cascadeSampleSizes;
};

#endif  // CASCADE_SSBO_GLSL
