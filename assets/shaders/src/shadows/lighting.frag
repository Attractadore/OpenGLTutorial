#version 460

#define CASCADE_SSBO_QUALIFIERS restrict readonly
#include "cascade_ssbo.glsl"
#include "shadow_setting_uniforms.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 normal_v;

layout(location = 0) out vec4 fColor;

layout(location = LIGHTING_FRAG_LIGHT_SHINE_DIR_LOCATION)
    uniform vec3 lightShine;
layout(location = LIGHTING_FRAG_CAMERA_POS_LOCATION)
    uniform vec3 cameraPos;

layout(binding = LIGHTING_FRAG_SHADOW_MAP_ARR_BINDING)
    uniform sampler2DArrayShadow lightShadowMapArray;
layout(binding = LIGHTING_FRAG_MSM_MOMENT_ARR_BINDING)
    uniform sampler2DArray msm_moment_array;

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

float msmShadowing(vec4 moments, float frag_depth) {
    /*
     * [ 1  b1 b2 ] [ c1 ]   [  1  ]
     * [ b1 b2 b3 ] [ c2 ] = [  z  ]
     * [ b2 b3 b4 ] [ c3 ]   [ z^2 ]
     *
     * [ 1  b1 b2 ]   [ 1    0  0 ][ D1 0  0  ][ 1 L21 L31 ]   [ D1    L21D1            L31D1                    ]
     * [ b1 b2 b3 ] = [ L21  1  0 ][ 0  D2 0  ][ 0  1  L32 ] = [ L21D1 L21^2 D1 + D2    L31L21D1 + L32D2         ]
     * [ b2 b3 b4 ]   [ L31 L32 1 ][ 0  0  D3 ][ 0  0   1  ]   [ L31D1 L31L21D1 + L32D2 L31^2 D1 + L32^2 D2 + D3 ]
     *
     * D1 = 1
     * L21 = b1
     * L31 = b2
     * D2 = b2 - L21^2 = b2 - b1^2
     * L31L21D1 + L32D2 = b3
     * L32 = (b3 - L31L21) / D2
     * D3 = b4 - b2^2 - (b3 - L31L21) * D2
     *
     * LDL^T c = b
     * DL^T c = y
     * L y = b
     * y1 = 1
     * L21 * y1 + y2 = z
     * y2 = z - b1
     * L31 * y1 + L32 * y2 + y3 = z^2
     * y3 = z^2 - L32 * y2 - L31
     * 
     * c3 = y3 / D3
     * c2 + L32c3 = y2 / D2
     * c2 = y2 / D2 - L32c3
     * c1 + L21c2 + L31c3 = y1 / D1
     * c1 = y1 / D1 - L21c2 - L31c3
     *
     */
    const float bias = 0.00003;
    moments = mix(moments, vec4(0.5f), bias);
    const float L21 = moments.x;
    const float L31 = moments.y;
    const float D2 = moments.y - L21 * L21;
    const float L32 = (moments.z - L31 * L21) / D2;
    const float D3 = moments.w - L31 * L31 - L32 * L32 * D2;

    vec3 c;
    c.x = 1;
    c.y = (frag_depth - L21) / D2;
    c.z = (frag_depth * frag_depth - L31 - L32 * c.y) / D3;

    c.y = c.y / c.z - L32;
    c.x = c.x / c.z - L21 * c.y - L31;

    // c /= c.z;

    // z^2 + c2 * z + c1 = 0
    float disc_sqrt = sqrt(c.y * c.y - 4.0f * c.x);
    float z2 = 0.5f * (-c.y - disc_sqrt);
    float z3 = 0.5f * (-c.y + disc_sqrt);

    if (frag_depth <= z2) {
        return 1.0f;
    }
    if (frag_depth <= z3) {
        return 1.0f - (frag_depth * z3 - moments.x * (frag_depth + z3) + moments.y) / (z3 - z2) / (frag_depth - z2);
    }
    return (z2 * z3 - moments.x * (z2 + z3) + moments.y) / (frag_depth - z2) / (frag_depth - z3);
}

float shadowing(vec3 v3LightDir, vec3 fragPos, vec3 fragNormal, int cascadeIndex) {
    float fCascadeSampleSize = cascadeSampleSizes[cascadeIndex];
    vec3 fragOffsetPos = fragPos + normalOffset(v3LightDir, fCascadeSampleSize, fragNormal);
    mat4 m4CascadeTrans = cascadeTransforms[cascadeIndex];
    vec3 fragLSPos = fragLSTransform(m4CascadeTrans, fragOffsetPos);
    switch (shadow_mode) {
        case SHADOW_MODE_STANDARD:
            return texture(lightShadowMapArray, vec4(fragLSPos.xy, cascadeIndex, fragLSPos.z));
        case SHADOW_MODE_MSM:
            return msmShadowing(texture(msm_moment_array, vec3(fragLSPos.xy, cascadeIndex)), fragLSPos.z);
    }
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
