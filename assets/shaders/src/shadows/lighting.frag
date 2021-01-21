#version 460 core

layout(location = 0) in VERT_OUT {
    vec3 pos;
    vec3 normal;
}
fIn;

layout(location = 0) out vec4 fColor;

layout(location = 20) uniform vec3 lightShine;

layout(location = 40) uniform vec3 cameraPos;

layout(binding = 0) uniform sampler2DArrayShadow lightShadowMapArray;

layout(binding = 1) restrict readonly buffer CascadePropertiesBuffer {
    mat4 cascadeTransforms[4];
    vec4 cascadeDepths;
    vec4 cascadeSampleSizes;
};

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

float shadowing(vec3 v3LightDir, vec3 fragPos, vec3 fragNormal, sampler2DArrayShadow shadowMapArray, int cascadeIndex) {
    float fCascadeSampleSize = cascadeSampleSizes[cascadeIndex];
    vec3 fragOffsetPos = fragPos + normalOffset(v3LightDir, fCascadeSampleSize, fragNormal);
    mat4 m4CascadeTrans = cascadeTransforms[cascadeIndex];
    vec3 fragLSPos = fragLSTransform(m4CascadeTrans, fragOffsetPos);
    return texture(shadowMapArray, vec4(fragLSPos.xy, cascadeIndex, fragLSPos.z));
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
    vec3 fragPos = fIn.pos;
    float fFragDepth = gl_FragCoord.z;
    vec3 fragNormal = normalize(fIn.normal);
    vec3 cameraDir = normalize(cameraPos - fragPos);
    vec3 v3LightDir = normalize(-lightShine);

    ivec4 iv4CascadeSelect = ivec4(1);

    ivec4 iv4Comp = ivec4(greaterThanEqual(vec4(fFragDepth), cascadeDepths));
    int iCascade = int(dot(iv4Comp, iv4CascadeSelect) - 1);

#if 1
    float fShadow = shadowing(v3LightDir, fragPos, fragNormal, lightShadowMapArray, iCascade);

    vec3 color = lighting(v3LightDir, cameraDir, fragNormal, fShadow);
#else
    vec3 colors[4] = {
        vec3(1.0F, 0.0F, 0.0F),
        vec3(1.0f, 1.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f),
    };
    vec3 color = colors[iCascade];
#endif

    fColor = vec4(color, 1.0f);
}
