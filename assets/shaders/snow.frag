#version 330 core
#define MAX_POINT_LIGHTS 10
#define MAX_DIR_LIGHTS 10
#define MAX_SPOT_LIGHTS 10
#include "lighting.glsl"

in VERT_OUT {
    vec3 pos;
    vec3 normal;
} fIn;

out vec4 fColor;

layout (std140) uniform LightsBlock {
    PointLight pointLights[MAX_POINT_LIGHTS]; // 800 bytes
    SpotLight spotLights[MAX_SPOT_LIGHTS]; // 1120 bytes
    DirLight dirLights[MAX_DIR_LIGHTS]; // 640 bytes
    int numPointLights; // 4 bytes
    int numSpotLights; // 4 bytes
    int numDirLights; // 4 bytes
} lights;

uniform vec3 snowDiffuse;
uniform vec3 snowSpecular;
uniform float snowShininess;
uniform vec3 cameraPos;

void main() {
    vec3 fragPos = fIn.pos;
    vec3 fragNormal = fIn.normal;
    MaterialColor fragMaterial;
    fragMaterial.diffuseColor = snowDiffuse;
    fragMaterial.specularColor = snowSpecular;
    fragMaterial.shininess = snowShininess;
    vec3 cameraDir = normalize(cameraPos - fragPos);

    vec3 resColor = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < min(lights.numPointLights, MAX_POINT_LIGHTS); i++){
        resColor += pointLightLighting(lights.pointLights[i], fragPos, fragNormal, cameraDir, fragMaterial);
    }
    for (int i = 0; i < min(lights.numSpotLights, MAX_SPOT_LIGHTS); i++){
        resColor += spotLightLighting(lights.spotLights[i], fragPos, fragNormal, cameraDir, fragMaterial);
    }
    for (int i = 0; i < min(lights.numDirLights, MAX_DIR_LIGHTS); i++){
        resColor += dirLightLighting(lights.dirLights[i], fragPos, fragNormal, cameraDir, fragMaterial);
    }
    fColor = vec4(resColor, 1.0f);
}
