#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D inputFrame;
uniform float strideX, strideY;
uniform bool bHorizontal;

const float kernelWeights[5] = float[](
    0.1f, 0.2f, 0.4f, 0.2f, 0.1f
);

void main(){
    vec2 samplePositions[5];
    if (bHorizontal){
        samplePositions = vec2[](
            vec2(2.0f * -strideX, 0.0f),
            vec2(1.0f * -strideX, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(1.0f * strideX, 0.0f),
            vec2(2.0f * strideX, 0.0f)
        );
    }
    else{
        samplePositions = vec2[](
            vec2(0.0f, 2.0f * -strideY),
            vec2(0.0f, 1.0f * -strideY),
            vec2(0.0f, 0.0f),
            vec2(0.0f, 1.0f * strideY),
            vec2(0.0f, 2.0f * strideY)
        );
    }

    vec3 color = vec3(0.0f);
    for (int i = 0; i < 5; i++){
        color += kernelWeights[i] * texture(inputFrame, fTex + samplePositions[i]).rgb;
    }
    fColor = vec4(color, 1.0f);
}
