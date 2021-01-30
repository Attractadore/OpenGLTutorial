#version 460
#extension GL_KHR_shader_subgroup_arithmetic : require

#define DEPTH_SSBO_QUALIFIERS coherent restrict
#include "depth_ssbo.glsl"

void main() {
    uvec2 work = uvec2(floatBitsToUint(gl_FragCoord.z));

    work.x = subgroupMin(work.x);
    work.y = subgroupMax(work.y);

    if (subgroupElect()) {
        atomicMin(minDepth, work.x);
        atomicMax(maxDepth, work.y);
    }
}
