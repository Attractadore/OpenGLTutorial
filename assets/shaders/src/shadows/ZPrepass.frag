#version 460
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(binding = 0, std430) coherent restrict buffer DepthDataBuffer {
    uint minDepth;
    uint maxDepth;
};

void main() {
    vec2 work = vec2(gl_FragCoord.z);

    work.x = subgroupMin(work.x);
    work.y = subgroupMax(work.y);

    if (subgroupElect()) {
        atomicMin(minDepth, uint((-1U) * work.x));
        atomicMax(maxDepth, uint((-1U) * work.y));
    }
}
