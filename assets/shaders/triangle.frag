#version 330 core
#define MAX_POINT_LIGHTS 10
#define MAX_DIR_LIGHTS 10
#define MAX_SPOT_LIGHTS 10

// Structs

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct LightColor {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct LightK {
    float c;
    float l;
    float q;
};

struct PointLight {
    vec3 position;
    LightColor color;
    LightK k;
};

struct DirLight {
    vec3 direction;
    LightColor color;
};

struct SpotLight {
    vec3 direction;
    vec3 position;
    LightColor color;
    LightK k;
    float inner;
    float outer;
};

// Dataflow

in vec3 fPos;
in vec3 fNormal;
in vec2 fTex;

out vec4 ofColor;

uniform vec3 cameraPos;
uniform Material material;
uniform int numPointLights, numDirLights, numSpotLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform DirLight dirLights[MAX_DIR_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

// Functions

float lightStrength(float distance, LightK k){
    return 1.0f / (k.c + distance * (k.l + distance * k.q));
}

vec3 generalLighting(LightColor color, vec3 lightDir){

    vec3 norm = normalize(fNormal);
    vec3 cameraDir = normalize(cameraPos - fPos);
    lightDir = normalize(lightDir);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambientLighting = color.ambient * vec3(texture(material.diffuse, fTex));

    float diffuseStrength = max(dot(lightDir, norm), 0.0f);
    vec3 diffuseLighting = color.diffuse * diffuseStrength * vec3(texture(material.diffuse, fTex));

    float specularStrength = pow(max(dot(reflectDir, cameraDir), 0.0f), material.shininess);
    vec3 specularLighting = color.specular * specularStrength * vec3(texture(material.specular, fTex));

    return ambientLighting + diffuseLighting + specularLighting;
}

vec3 dirLightLighting(DirLight dl){
    return generalLighting(dl.color, -dl.direction);
}

vec3 pointLightLighting(PointLight pl){
    vec3 resLighting = generalLighting(pl.color, pl.position - fPos);
    resLighting *= lightStrength(length(pl.position - fPos), pl.k);
    return resLighting;
}

vec3 spotLightLighting(SpotLight sl){
    vec3 lightDir = normalize(sl.position - fPos);
    vec3 resLighting = generalLighting(sl.color, lightDir);
    resLighting *= lightStrength(length(sl.position - fPos), sl.k);
    float lightAngle = dot(-lightDir, sl.direction);
    resLighting *= clamp((lightAngle - sl.outer) / (sl.inner - sl.outer), 0.0f, 1.0f);
    return resLighting;
}

void main()
{
    vec3 resColor = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numPointLights; i++){
        resColor += pointLightLighting(pointLights[i]);
    }
    for (int i = 0; i < numSpotLights; i++){
        resColor += spotLightLighting(spotLights[i]);
    }
    for (int i = 0; i < numDirLights; i++){
        resColor += dirLightLighting(dirLights[i]);
    }
    ofColor = vec4(resColor, 1.0f);
}
