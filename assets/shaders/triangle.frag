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

struct LightColor { // 48 bytes
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct LightK { // 16 bytes
    float c;
    float l;
    float q;
};

struct PointLight { // 80 bytes
    LightColor color;
    LightK k;
    vec3 position;
};

struct DirLight { // 64 bytes
    LightColor color;
    vec3 direction;
};

struct SpotLight { // 112 bytes
    LightColor color;
    LightK k;
    vec3 position;
    vec3 direction;
    float inner;
    float outer;
};

// Dataflow

in VERT_OUT {
    vec3 pos;
    vec3 normal;
    vec2 tex;
} fIn;

out vec4 fColor;

uniform vec3 cameraPos;
uniform Material material;

layout (std140) uniform LightsBlock {
    PointLight pointLights[MAX_POINT_LIGHTS]; // 800 bytes
    SpotLight spotLights[MAX_SPOT_LIGHTS]; // 1120 bytes
    DirLight dirLights[MAX_DIR_LIGHTS]; // 640 bytes
    int numPointLights; // 4 bytes
    int numSpotLights; // 4 bytes
    int numDirLights; // 4 bytes
} lights;

// Functions

float lightStrength(float distance, LightK k){
    return 1.0f / (k.c + distance * (k.l + distance * k.q));
}

vec3 generalLighting(LightColor color, vec3 lightDir){

    vec3 norm = normalize(fIn.normal);
    vec3 cameraDir = normalize(cameraPos - fIn.pos);
    lightDir = normalize(lightDir);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambientLighting = color.ambient * vec3(texture(material.diffuse, fIn.tex));

    float diffuseStrength = max(dot(lightDir, norm), 0.0f);
    vec3 diffuseLighting = color.diffuse * diffuseStrength * vec3(texture(material.diffuse, fIn.tex));

    float specularStrength = pow(max(dot(reflectDir, cameraDir), 0.0f), material.shininess);
    vec3 specularLighting = color.specular * specularStrength * vec3(texture(material.specular, fIn.tex));

    return ambientLighting + diffuseLighting + specularLighting;
}

vec3 dirLightLighting(DirLight dl){
    return generalLighting(dl.color, -dl.direction);
}

vec3 pointLightLighting(PointLight pl){
    vec3 resLighting = generalLighting(pl.color, pl.position - fIn.pos);
    resLighting *= lightStrength(length(pl.position - fIn.pos), pl.k);
    return resLighting;
}

vec3 spotLightLighting(SpotLight sl){
    vec3 lightDir = normalize(sl.position - fIn.pos);
    vec3 resLighting = generalLighting(sl.color, lightDir);
    resLighting *= lightStrength(length(sl.position - fIn.pos), sl.k);
    float lightAngle = dot(-lightDir, sl.direction);
    resLighting *= clamp((lightAngle - sl.outer) / (sl.inner - sl.outer), 0.0f, 1.0f);
    return resLighting;
}

void main()
{
    vec3 resColor = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < lights.numPointLights; i++){
        resColor += pointLightLighting(lights.pointLights[i]);
    }
    for (int i = 0; i < lights.numSpotLights; i++){
        resColor += spotLightLighting(lights.spotLights[i]);
    }
    for (int i = 0; i < lights.numDirLights; i++){
        resColor += dirLightLighting(lights.dirLights[i]);
    }
    fColor = vec4(resColor, 1.0f);
}
