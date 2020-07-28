#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

// Structs

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

const vec3 offsets[9] = vec3[](
    vec3(1.0f, 1.0f, 1.0f),
    vec3(-1.0f, 1.0f, 1.0f),
    vec3(-1.0f, -1.0f, 1.0f),
    vec3(1.0f, -1.0f, 1.0f),
    vec3(0.0f),
    vec3(1.0f, 1.0f, -1.0f),
    vec3(-1.0f, 1.0f, -1.0f),
    vec3(-1.0f, -1.0f, -1.0f),
    vec3(1.0f, -1.0f, -1.0f));

const float weights[9] = float[](0.5, 0.5, 0.5f, 0.5f, 1.0f, 0.5, 0.5f, 0.5f, 0.5f);

const float offsetScale = 0.01f;

// Functions

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

float lightShadowing2D(sampler2DArrayShadow shadowMapArray, int index, vec3 fragPos, mat4 lightTransform) {
    float lightFactor = 0.0f;
    float normFactor = 0.0f;
    for (int i = 0; i < 9; i++) {
        vec3 lssFragPos = fragLSSTrans(fragPos + offsets[i] * offsetScale, lightTransform);
        vec4 fragShadowTex = vec4(lssFragPos.xy, index, lssFragPos.z);
        lightFactor += weights[i] * texture(shadowMapArray, fragShadowTex);
        normFactor += weights[i];
    }
    return lightFactor / normFactor;
}

float lightShadowingCube(samplerCubeArrayShadow shadowCubeMapArray, int index, vec3 fragPos, mat4 lightTransform, float lightNear, float lightFar) {
    float lightFactor = 0.0f;
    float normFactor = 0.0f;
    for (int i = 0; i < 9; i++) {
        vec3 lsFragPos = (lightTransform * vec4(fragPos + offsets[i] * offsetScale, 1.0f)).xyz;
        vec4 fragShadowTex = vec4(lsFragPos, index);
        lsFragPos = abs(lsFragPos);
        float z = max(lsFragPos.x, max(lsFragPos.y, lsFragPos.z));
        float fragDepth = (lightNear / z - 1.0f) / (lightNear / lightFar - 1.0f);
        lightFactor += weights[i] * texture(shadowCubeMapArray, fragShadowTex, fragDepth);
        normFactor += weights[i];
    }
    return lightFactor / normFactor;
}

#endif
