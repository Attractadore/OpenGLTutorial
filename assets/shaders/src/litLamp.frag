#version 460 core
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout (location = 0) in vec3 fPos;

layout (location = 0) out vec4 fColor;

layout (location = 4) uniform vec3 lightColor;
layout (location = 5) uniform LightAttenuation lightAtt;
layout (location = 7) uniform bool bUseCone;
layout (location = 8) uniform float inner;
layout (location = 9) uniform float outer;

void main(){
    vec3 resColor = normalize(lightColor) * disAtt(length(fPos), lightAtt);
    if (bUseCone){
        float fCos = dot(normalize(fPos), vec3(0.0f, 0.0f, -1.0f));
        resColor *= angAtt(fCos, inner, outer);
    }
    fColor = vec4(resColor, 1.0f);
}