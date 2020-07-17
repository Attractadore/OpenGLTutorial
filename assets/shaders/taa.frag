#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2DArray frames;
uniform int numFrames;

void main() {
    vec3 color = vec3(0.0f);
    for (int i = 0; i < numFrames; i++) {
        color += texture(frames, vec3(fTex, i)).rgb;
    }
    color /= numFrames;
    fColor = vec4(color, 1.0f);
}
