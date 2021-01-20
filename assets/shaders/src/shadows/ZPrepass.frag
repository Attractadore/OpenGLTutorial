#version 460
#extension GL_KHR_shader_subgroup_arithmetic : require

layout(binding = 0) coherent restrict buffer OutData {
    uint minDepth;
    uint maxDepth;
}
outData;

void main() {
    vec2 work = vec2(gl_FragCoord.z);

    work.x = subgroupMin(work.x);
    work.y = subgroupMax(work.y);

    if (subgroupElect()) {
        atomicMin(outData.minDepth, uint((-1u) * work.x));
        atomicMax(outData.maxDepth, uint((-1u) * work.y));
    }
}
