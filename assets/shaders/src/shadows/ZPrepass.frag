#version 460
#extension GL_KHR_shader_subgroup_arithmetic : require

#define DEPTH_SSBO_QUALIFIERS coherent restrict
#include "depth_ssbo.glsl"
#include "shadow_setting_uniforms.glsl"

void main() {
    if (b_shadow_cascade_depth_adjust) {
        uvec2 work = uvec2(floatBitsToUint(gl_FragCoord.z));

        work.x = subgroupMin(work.x);
        work.y = subgroupMax(work.y);

        if (subgroupElect()) {
            atomicMin(minDepth, work.x);
            atomicMax(maxDepth, work.y);
        }
    }
}
