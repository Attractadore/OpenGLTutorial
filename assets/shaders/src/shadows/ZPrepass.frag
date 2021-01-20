#version 460

layout(location = 0) out vec2 depth;

void main() {
    depth = vec2(gl_FragCoord.z);
}
