#version 460

layout(location = 0) out vec4 moments;

void main() {
    const float depth = gl_FragCoord.z;
    const float depth_squared = depth * depth;
    moments = vec4(
        depth,
        depth_squared,
        depth_squared * depth,
        depth_squared * depth_squared);
}
