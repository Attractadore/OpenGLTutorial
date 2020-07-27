#version 330 core
#define MAX_POINT_LIGHTS 10
#define MAX_DIR_LIGHTS 10
#define MAX_SPOT_LIGHTS 10
#define DISCARD_THRESHOLD 0.01f
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
    PointLight pointLights[MAX_POINT_LIGHTS];     // 640 bytes
    SpotLight spotLights[MAX_SPOT_LIGHTS];        // 960 bytes
    DirLight dirLights[MAX_DIR_LIGHTS];           // 640 bytes
    mat4 pointLightTransforms[MAX_POINT_LIGHTS];  // 640 bytes
    mat4 spotLightTransforms[MAX_SPOT_LIGHTS];    // 640 bytes
    mat4 dirLightTransforms[MAX_DIR_LIGHTS];      // 640 bytes
    int numPointLights;                           // 4 bytes
    int numSpotLights;                            // 4 bytes
    int numDirLights;                             // 4 bytes
}
lights;

uniform vec3 cameraPos;
uniform Material material;

uniform sampler2DArrayShadow spotLightShadowMapArray;
uniform sampler2DArrayShadow dirLightShadowMapArray;

void main() {
    float alpha = texture(material.diffuseMap, fIn.tex).a;
    if (alpha < DISCARD_THRESHOLD) {
        discard;
    }

    vec3 fragPos = fIn.pos;
    vec3 fragNormal = normalize(fIn.normal);
    MaterialColor fragMaterial;
    fragMaterial.diffuseColor = texture(material.diffuseMap, fIn.tex).rgb;
    fragMaterial.specularColor = texture(material.specularMap, fIn.tex).rgb;
    fragMaterial.shininess = material.shininess;
    vec3 cameraDir = normalize(cameraPos - fragPos);

    vec3 resColor = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < min(lights.numPointLights, MAX_POINT_LIGHTS); i++) {
        resColor += pointLightLighting(lights.pointLights[i], fragPos, fragNormal, cameraDir, fragMaterial, 1.0f);
    }
    for (int i = 0; i < min(lights.numSpotLights, MAX_SPOT_LIGHTS); i++) {
        resColor += spotLightLighting(lights.spotLights[i], fragPos, fragNormal, cameraDir, fragMaterial,
                                      lightShadowing2D(spotLightShadowMapArray, i, fragPos, lights.spotLightTransforms[i]));
    }
    for (int i = 0; i < min(lights.numDirLights, MAX_DIR_LIGHTS); i++) {
        resColor += dirLightLighting(lights.dirLights[i], fragPos, fragNormal, cameraDir, fragMaterial,
                                     lightShadowing2D(dirLightShadowMapArray, i, fragPos, lights.dirLightTransforms[i]));
    }
    fColor = vec4(resColor, alpha);
}
