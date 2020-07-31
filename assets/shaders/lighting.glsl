#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

struct LightColor {  // 48 bytes
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {  // 64 bytes
    LightColor color;
    vec3 position;
    float radius;
};

struct DirLight {  // 64 bytes
    LightColor color;
    vec3 direction;
};

struct SpotLight {  // 96 bytes
    LightColor color;
    vec3 position;
    float radius;
    vec3 direction;
    float inner;
    float outer;
};

struct MaterialColor {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
};

float lightDistanceAtt(float distance, float radius) {
    return pow(radius / max(distance, radius), 2.0f);
}

float lightAngleAtt(float angle, float inner, float outer) {
    return clamp((angle - outer) / (inner - outer), 0.0f, 1.0f);
}

vec3 generalLighting(LightColor color, vec3 fragNormal, vec3 lightDir, vec3 cameraDir, MaterialColor fragMaterial, float lightingFactor) {
    vec3 halfwayDir = normalize(lightDir + cameraDir);

    vec3 ambientLighting = color.ambient * fragMaterial.diffuseColor;

    float diffuseStrength = max(dot(lightDir, fragNormal), 0.0f);
    vec3 diffuseLighting = color.diffuse * diffuseStrength * fragMaterial.diffuseColor;

    float specularStrength = pow(max(dot(halfwayDir, fragNormal), 0.0f), fragMaterial.shininess);
    vec3 specularLighting = color.specular * specularStrength * fragMaterial.specularColor;

    return ambientLighting + lightingFactor * (diffuseLighting + specularLighting);
}

vec3 dirLightLighting(DirLight dl, vec3 fragPos, vec3 fragNormal, vec3 cameraDir, MaterialColor fragMaterial, float lightingFactor) {
    return generalLighting(dl.color, fragNormal, normalize(-dl.direction), cameraDir, fragMaterial, lightingFactor);
}

vec3 pointLightLighting(PointLight pl, vec3 fragPos, vec3 fragNormal, vec3 cameraDir, MaterialColor fragMaterial, float lightingFactor) {
    vec3 resLighting = generalLighting(pl.color, fragNormal, normalize(pl.position - fragPos), cameraDir, fragMaterial, lightingFactor);
    resLighting *= lightDistanceAtt(length(pl.position - fragPos), pl.radius);
    return resLighting;
}

vec3 spotLightLighting(SpotLight sl, vec3 fragPos, vec3 fragNormal, vec3 cameraDir, MaterialColor fragMaterial, float lightingFactor) {
    vec3 lightDir = normalize(sl.position - fragPos);
    vec3 resLighting = generalLighting(sl.color, fragNormal, lightDir, cameraDir, fragMaterial, lightingFactor);
    resLighting *= lightDistanceAtt(length(sl.position - fragPos), sl.radius);
    resLighting *= lightAngleAtt(dot(-lightDir, sl.direction), sl.inner, sl.outer);
    return resLighting;
}

vec3 fragLSSTrans(vec3 fragPos, mat4 lightTransform) {
    vec4 lsFragPos = lightTransform * vec4(fragPos, 1.0f);
    lsFragPos /= lsFragPos.w;
    return lsFragPos.xyz * 0.5f + 0.5f;
}

float fragCubeDepth(vec3 fragPos, mat4 lightTransform, float lightNear, float lightFar) {
    vec3 lsFragPos = abs((lightTransform * vec4(fragPos, 1.0f)).xyz);
    float z = max(lsFragPos.x, max(lsFragPos.y, lsFragPos.z));
    return (lightNear / z - 1.0f) / (lightNear / lightFar - 1.0f);
}

vec3 calculateNormalOffset(vec3 fragNormal, float fragDepth, vec3 lightDir, float minSampleSize, float maxSampleSize) {
    float offsetScale = sin(acos(dot(fragNormal, lightDir))) * mix(minSampleSize, maxSampleSize, pow(fragDepth, 50.0f)) / sqrt(2.0f) + 0.01f;
    return offsetScale * fragNormal;
}

float lightShadowing2D(sampler2DArrayShadow shadowMapArray, int index, vec3 fragPos, vec3 fragNormal, vec3 lightDir, mat4 lightTransform, float minSampleSize, float maxSampleSize) {
    float fragDepth = fragLSSTrans(fragPos, lightTransform).z;
    vec3 fragOffset = calculateNormalOffset(fragNormal, fragDepth, lightDir, minSampleSize, maxSampleSize);

    vec3 lssFragSamplePos = fragLSSTrans(fragPos + fragOffset, lightTransform);
    vec4 fragShadowTex = vec4(lssFragSamplePos.xy, index, lssFragSamplePos.z);
    return texture(shadowMapArray, fragShadowTex);
}

float lightShadowingCube(samplerCubeArrayShadow shadowCubeMapArray, int index, vec3 fragPos, vec3 fragNormal, vec3 lightDir, mat4 lightTransform, float lightNear, float lightFar, float minSampleSize, float maxSampleSize) {
    float fragDepth = fragCubeDepth(fragPos, lightTransform, lightNear, lightFar);
    vec3 fragOffset = calculateNormalOffset(fragNormal, fragDepth, lightDir, minSampleSize, maxSampleSize);

    vec3 lsFragSamplePos = (lightTransform * vec4(fragPos + fragOffset, 1.0f)).xyz;
    float fragSampleDepth = fragCubeDepth(lsFragSamplePos, mat4(1.0f), lightNear, lightFar);
    vec4 fragShadowTex = vec4(lsFragSamplePos, index);
    return texture(shadowCubeMapArray, fragShadowTex, fragSampleDepth);
}

#endif
