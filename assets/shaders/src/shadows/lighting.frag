#version 460 core

in VERT_OUT {
    vec3 pos;
    vec3 normal;
} fIn;

layout(location = 0) out vec4 fColor;

layout(location = 20) uniform vec3 lightShine; 
layout(location = 21) uniform mat4 lightProjView;
layout(location = 22) uniform float lightSampleSize;

layout(location = 30) uniform vec3 cameraPos; 

layout(binding = 0) uniform sampler2DShadow lightShadowMap;

struct DirLight {
    vec3 dir;
    mat4 trans;
    float sampleSize;
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

float shadowing(DirLight light, sampler2DShadow shadowMap, vec3 fragPos, vec3 fragNormal) {
    vec3 fragOffsetPos = fragPos + normalOffset(light.dir, light.sampleSize, fragNormal);
    vec3 fragLSPos = fragLSTransform(light.trans, fragOffsetPos);
    return texture(shadowMap, fragLSPos);
}

vec3 lighting(DirLight light, sampler2DShadow shadowMap, vec3 cameraDir, vec3 fragPos, vec3 fragNormal) {
    vec3 halfwayDir = normalize(cameraDir + light.dir);

    float ambient = 0.1f;
    float diffuse = max(dot(fragNormal, light.dir), 0.0f);
    float specular = pow(max(dot(fragNormal, halfwayDir), 0.0f), 64.0f);
    float shadow = shadowing(light, shadowMap, fragPos, fragNormal);
    float lightingStrength = ambient + shadow * (diffuse + specular);
    
    return vec3(lightingStrength);
}

void main() {
    vec3 fragPos = fIn.pos;
    vec3 fragNormal = normalize(fIn.normal);
    vec3 cameraDir = normalize(cameraPos - fragPos);

    DirLight light = {
        normalize(-lightShine),
        lightProjView,
        lightSampleSize
    };

    vec3 color = lighting(light, lightShadowMap, cameraPos, fragPos, fragNormal);
    fColor = vec4(color, 1.0f);
}
