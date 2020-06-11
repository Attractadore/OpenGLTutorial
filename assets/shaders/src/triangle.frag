#version 460 core
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

const int MAX_POINT_LIGHTS = 10;
const int MAX_SPOT_LIGHTS = 10;
const int MAX_DIR_LIGHTS = 1;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct LightColor {
    float ambient;
    vec3 diffuse;
    float specular;
};

struct DirLight {
    vec3 direction;
    LightColor color;
};

struct PointLight {
    vec3 position;
    LightAttenuation attenuation;
    LightColor color;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float inner;
    float outer;
    LightAttenuation attenuation;
    LightColor color;
};

layout (location = 0) in vec3 fPos;
layout (location = 1) in vec3 fNormal;
layout (location = 2) in vec2 fTex;

layout (location = 0) out vec4 ofColor;

layout (location = 4)
uniform Material material;
layout (location = 7)
uniform vec3 cameraPos;
layout (location = 8)
uniform int numDirLights;
layout (location = 9)
uniform int numPointLights;
layout (location = 10)
uniform int numSpotLights;
layout (location = 11)
uniform DirLight dirLights[MAX_DIR_LIGHTS];
layout (location = 11 + MAX_DIR_LIGHTS * 4)
uniform PointLight pointLights[MAX_POINT_LIGHTS];
layout (location = 11 + MAX_DIR_LIGHTS * 4 + MAX_POINT_LIGHTS * 6)
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

vec3 light(vec3 lightDir, vec3 norm, vec3 viewDir, LightColor lightColor){
    lightDir = normalize(lightDir);
    norm = normalize(norm);
    vec3 reflectDir = reflect(-lightDir, norm);
    viewDir = normalize(viewDir);

    float diffuseStrength = clamp(dot(lightDir, norm), 0.0f, 1.0f) + lightColor.ambient;
    float specularStrength = pow(clamp(dot(reflectDir, viewDir), 0.0f, 1.0f), material.shininess) * lightColor.specular;
    return (texture(material.diffuse, fTex).rgb * diffuseStrength + texture(material.specular, fTex).rgb * specularStrength) * lightColor.diffuse;
}

vec3 lightDirLight(DirLight dl){
    return light(-dl.direction, fNormal, cameraPos - fPos, dl.color);
}

vec3 lightPointLight(PointLight pl){
    vec3 lightDir = pl.position - fPos;
    float distFactor = disAtt(length(lightDir), pl.attenuation);
    return distFactor * light(lightDir, fNormal, cameraPos - fPos, pl.color);
}

vec3 lightSpotLight(SpotLight sl){
    vec3 lightDir = sl.position - fPos;
    float distFactor = disAtt(length(lightDir), sl.attenuation);
    float fCos = dot(normalize(-lightDir), sl.direction);
    float angleFactor = angAtt(fCos, sl.inner, sl.outer);
    return distFactor * angleFactor * light(lightDir, fNormal, cameraPos - fPos, sl.color);
}

void main()
{
    vec3 resColor = vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < min(MAX_DIR_LIGHTS, numDirLights); i++){
        resColor += lightDirLight(dirLights[i]);
    }
    for (int i = 0; i < min(MAX_POINT_LIGHTS, numPointLights); i++){
        resColor += lightPointLight(pointLights[i]);
    }
    for (int i = 0; i < min(MAX_SPOT_LIGHTS, numSpotLights); i++){
        resColor += lightSpotLight(spotLights[i]);
    }
    ofColor = vec4(resColor, 1.0f);
}
