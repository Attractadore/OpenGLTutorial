#version 460

#define CASCADE_SSBO_QUALIFIERS restrict readonly
#include "cascade_ssbo.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 normal_v;

layout(location = 0) out vec4 fColor;

layout(location = LIGHTING_FRAG_LIGHT_SHINE_DIR_LOCATION)
    uniform vec3 lightShine;
layout(location = LIGHTING_FRAG_CAMERA_POS_LOCATION)
    uniform vec3 cameraPos;

layout(binding = LIGHTING_FRAG_SHADOW_MAP_ARR_BINDING)
    uniform sampler2DArrayShadow lightShadowMapArray;

vec3 normalOffset(vec3 lightDir, float lightSampleSize, vec3 fragNormal) {
    float lightNormalCos = dot(lightDir, fragNormal);
    float lightNormalSin = sqrt(1.0f - lightNormalCos * lightNormalCos);
    float offsetScale = lightNormalSin * lightSampleSize / sqrt(2.0f) + 0.02f;
    return offsetScale * fragNormal;
}

vec3 fragLSTransform(mat4 lightTrans, vec3 fragPos) {
    vec4 lsPos = lightTrans * vec4(fragPos, 1.0f);
    lsPos /= lsPos.w;
    return 0.5f * (lsPos.xyz + 1.0f);
}

float shadowing(vec3 v3LightDir, vec3 fragPos, vec3 fragNormal, int cascadeIndex) {
    float fCascadeSampleSize = cascadeSampleSizes[cascadeIndex];
    vec3 fragOffsetPos = fragPos + normalOffset(v3LightDir, fCascadeSampleSize, fragNormal);
    mat4 m4CascadeTrans = cascadeTransforms[cascadeIndex];
    vec3 fragLSPos = fragLSTransform(m4CascadeTrans, fragOffsetPos);
    return texture(lightShadowMapArray, vec4(fragLSPos.xy, cascadeIndex, fragLSPos.z));
}

vec3 lighting(vec3 v3LightDir, vec3 cameraDir, vec3 fragNormal, float shadow) {
    vec3 halfwayDir = normalize(cameraDir + v3LightDir);

    float ambient = 0.1f;
    float diffuse = max(dot(fragNormal, v3LightDir), 0.0f);
    float specular = pow(max(dot(fragNormal, halfwayDir), 0.0f), 64.0f);
    float lightingStrength = ambient + shadow * (diffuse + specular);

    return vec3(lightingStrength);
}

void main() {
    float fFragDepth = gl_FragCoord.z;
    vec3 fragNormal = normalize(normal_v);
    vec3 cameraDir = normalize(cameraPos - fragPos);
    vec3 v3LightDir = normalize(-lightShine);

    ivec4 iv4CascadeSelect = ivec4(1);

    ivec4 iv4Comp = ivec4(greaterThanEqual(vec4(fFragDepth), cascadeDepths));
    int iCascade = int(dot(iv4Comp, iv4CascadeSelect) - 1);

    float fShadow = shadowing(v3LightDir, fragPos, fragNormal, iCascade);
    vec3 color = lighting(v3LightDir, cameraDir, fragNormal, fShadow);

    fColor = vec4(color, 1.0f);
}
