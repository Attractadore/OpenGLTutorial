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

// Functions

float lightDistanceAtt(float distance, float radius) {
    return pow(radius / max(distance, radius), 2.0f);
}

float lightAngleAtt(float angle, float inner, float outer) {
    return clamp((angle - outer) / (inner - outer), 0.0f, 1.0f);
}

vec3 generalLighting(LightColor color, vec3 fragNormal, vec3 lightDir, vec3 cameraDir, MaterialColor fragMaterial) {
    vec3 halfwayDir = normalize(lightDir + cameraDir);

    vec3 ambientLighting = color.ambient * fragMaterial.diffuseColor;

    float diffuseStrength = max(dot(lightDir, fragNormal), 0.0f);
    vec3 diffuseLighting = color.diffuse * diffuseStrength * fragMaterial.diffuseColor;

    float specularStrength = pow(max(dot(halfwayDir, fragNormal), 0.0f), fragMaterial.shininess);
    vec3 specularLighting = color.specular * specularStrength * fragMaterial.specularColor;

    return ambientLighting + diffuseLighting + specularLighting;
}

vec3 dirLightLighting(DirLight dl, vec3 fragPos, vec3 fragNormal, vec3 cameraDir, MaterialColor fragMaterial) {
    return generalLighting(dl.color, fragNormal, normalize(-dl.direction), cameraDir, fragMaterial);
}

vec3 pointLightLighting(PointLight pl, vec3 fragPos, vec3 fragNormal, vec3 cameraDir, MaterialColor fragMaterial) {
    vec3 resLighting = generalLighting(pl.color, fragNormal, normalize(pl.position - fragPos), cameraDir, fragMaterial);
    resLighting *= lightDistanceAtt(length(pl.position - fragPos), pl.radius);
    return resLighting;
}

vec3 spotLightLighting(SpotLight sl, vec3 fragPos, vec3 fragNormal, vec3 cameraDir, MaterialColor fragMaterial) {
    vec3 lightDir = normalize(sl.position - fragPos);
    vec3 resLighting = generalLighting(sl.color, fragNormal, lightDir, cameraDir, fragMaterial);
    resLighting *= lightDistanceAtt(length(sl.position - fragPos), sl.radius);
    resLighting *= lightAngleAtt(dot(-lightDir, sl.direction), sl.inner, sl.outer);
    return resLighting;
}

#endif
