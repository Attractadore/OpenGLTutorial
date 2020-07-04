#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D inputFrame;
uniform float strideX, strideY;
uniform float intencity;

const float kernelWeights[9] = float[](
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f,
    2.0f / 16.0f, 4.0f / 16.0f, 2.0f / 16.0f,
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f
);

void main(){
    vec2 samplePositions[9] = vec2[](
        vec2(-strideX, strideY),
        vec2(0.0f, strideY),
        vec2(strideX, strideY),
        vec2(-strideX, 0.0f),
        vec2(0.0f, 0.0f),
        vec2(strideX, 0.0f),
        vec2(-strideX, -strideY),
        vec2(0.0f, -strideY),
        vec2(strideX, -strideY)
    );

    vec3 color = vec3(0.0f);
    for (int i = 0; i < 9; i++){
        color += kernelWeights[i] * pow(texture(inputFrame, fTex + samplePositions[i]).rgb, vec3(intencity));
    }
    fColor = vec4(color + texture(inputFrame, fTex).rgb, 1.0f);
}