#ifndef DEPTH_SSBO_GLSL
#define DEPTH_SSBO_GLSL

#ifndef DEPTH_SSBO_QUALIFIERS
#error
#endif

#include "indices.glsl"

layout(binding = DEPTH_SSBO_BINDING, std430)
    DEPTH_SSBO_QUALIFIERS buffer depth_ssbo {
    uint minDepth;
    uint maxDepth;
};

#endif  // DEPTH_SSBO_GLSL
