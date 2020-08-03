#version 330 core
#extension GL_ARB_texture_cube_map_array : require
#define MAX_POINT_LIGHTS 10
#define MAX_DIR_LIGHTS 10
#define MAX_SPOT_LIGHTS 10
#define MAX_DIR_LIGHT_CASCADES 4
#include "lighting.glsl"

struct Material {
    sampler2D diffuseMap;
    sampler2D specularMap;
    float shininess;
};

in VERT_OUT {
    vec3 pos;
    vec3 normal;
    vec2 tex;
}
fIn;

out vec4 fColor;

layout(std140) uniform LightsBlock {
    PointLight pointLights[MAX_POINT_LIGHTS];                          // 640 bytes
    SpotLight spotLights[MAX_SPOT_LIGHTS];                             // 960 bytes
    DirLight dirLights[MAX_DIR_LIGHTS];                                // 640 bytes
    mat4 pointLightTransforms[MAX_POINT_LIGHTS];                       // 640 bytes
    mat4 spotLightTransforms[MAX_SPOT_LIGHTS];                         // 640 bytes
    mat4 dirLightTransforms[MAX_DIR_LIGHTS * MAX_DIR_LIGHT_CASCADES];  // 2560 bytes
    int numPointLights;                                                // 4 bytes
    int numSpotLights;                                                 // 4 bytes
    int numDirLights;                                                  // 4 bytes
}
lights;

uniform float pointLightMinSampleSizes[MAX_POINT_LIGHTS];
uniform float pointLightMaxSampleSizes[MAX_POINT_LIGHTS];
uniform float spotLightMinSampleSizes[MAX_SPOT_LIGHTS];
uniform float spotLightMaxSampleSizes[MAX_SPOT_LIGHTS];
uniform float dirLightSampleSizes[MAX_DIR_LIGHTS * MAX_DIR_LIGHT_CASCADES];

uniform vec4 dirLightCascadeNearDepths;
uniform vec4 dirLightCascadeFarDepths;
uniform int dirLightNumCascades;

uniform vec3 cameraPos;
uniform Material material;

uniform samplerCubeArrayShadow pointLightShadowMapArray;
uniform sampler2DArrayShadow spotLightShadowMapArray;
uniform sampler2DArrayShadow dirLightShadowMapArray;

void main() {
    vec3 fragPos = fIn.pos;
    vec3 fragNormal = normalize(fIn.normal);
    MaterialColor fragMaterial;
    fragMaterial.diffuseColor = texture(material.diffuseMap, fIn.tex).rgb;
    fragMaterial.specularColor = texture(material.specularMap, fIn.tex).rgb;
    fragMaterial.shininess = material.shininess;
    vec3 cameraDir = normalize(cameraPos - fragPos);

    vec3 resColor = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < min(lights.numPointLights, MAX_POINT_LIGHTS); i++) {
        PointLight pl = lights.pointLights[i];
        vec3 lightDir = normalize(pl.position - fragPos);
        resColor += pointLightLighting(pl, fragPos, fragNormal, cameraDir, fragMaterial,
                                       lightShadowingCube(pointLightShadowMapArray, i, fragPos, fragNormal, lightDir, lights.pointLightTransforms[i], pl.radius, 100.0f, pointLightMinSampleSizes[i], pointLightMaxSampleSizes[i]));
    }
    for (int i = 0; i < min(lights.numSpotLights, MAX_SPOT_LIGHTS); i++) {
        SpotLight sl = lights.spotLights[i];
        vec3 lightDir = normalize(sl.position - fragPos);
        resColor += spotLightLighting(sl, fragPos, fragNormal, cameraDir, fragMaterial,
                                      lightShadowing2D(spotLightShadowMapArray, i, fragPos, fragNormal, lightDir, lights.spotLightTransforms[i], spotLightMinSampleSizes[i], spotLightMaxSampleSizes[i]));
    }

    ivec4 dirLightCascadeSelection = ivec4(
        dirLightNumCascades > 0,
        dirLightNumCascades > 1,
        dirLightNumCascades > 2,
        dirLightNumCascades > 3);

    for (int i = 0; i < min(lights.numDirLights, MAX_DIR_LIGHTS); i++) {
        DirLight dl = lights.dirLights[i];
        vec3 lightDir = normalize(-dl.direction);

        ivec4 comparison = ivec4(greaterThanEqual(vec4(gl_FragCoord.z), dirLightCascadeNearDepths));
        int cascadeFarIndex = int(dot(dirLightCascadeSelection, comparison)) - 1;
        comparison = ivec4(lessThanEqual(vec4(gl_FragCoord.z), dirLightCascadeFarDepths));
        int cascadeNearIndex = dirLightNumCascades - int(dot(dirLightCascadeSelection, comparison));

        int iCascadePropertiesFarIndex = dirLightNumCascades * i + cascadeFarIndex;
        mat4 m4CascadeFarTransform = lights.dirLightTransforms[iCascadePropertiesFarIndex];
        float fCascadeSampleSizeFar = dirLightSampleSizes[iCascadePropertiesFarIndex];

        float lightFactor = lightShadowing2D(dirLightShadowMapArray, iCascadePropertiesFarIndex, fragPos, fragNormal, lightDir, m4CascadeFarTransform, fCascadeSampleSizeFar, fCascadeSampleSizeFar);

        if (cascadeNearIndex < cascadeFarIndex) {
            int iCascadePropertiesNearIndex = dirLightNumCascades * i + cascadeNearIndex;
            mat4 m4CascadeNearTransform = lights.dirLightTransforms[iCascadePropertiesNearIndex];
            float fCascadeSampleSizeNear = dirLightSampleSizes[iCascadePropertiesNearIndex];

            float lightFactorNear = lightShadowing2D(dirLightShadowMapArray, iCascadePropertiesNearIndex, fragPos, fragNormal, lightDir, m4CascadeNearTransform, fCascadeSampleSizeNear, fCascadeSampleSizeNear);
            float mixFactor = clamp((gl_FragCoord.z - dirLightCascadeNearDepths[cascadeFarIndex]) / (dirLightCascadeFarDepths[cascadeNearIndex] - dirLightCascadeNearDepths[cascadeFarIndex]), 0.0f, 1.0f);
            lightFactor = mix(lightFactorNear, lightFactor, mixFactor);
        }

        resColor += dirLightLighting(dl, fragPos, fragNormal, cameraDir, fragMaterial, lightFactor);
    }

    float alpha = texture(material.diffuseMap, fIn.tex).a;
    fColor = vec4(resColor, alpha);
}
