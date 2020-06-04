#version 330 core
in vec3 fPos;
in vec3 fNormal;
in vec2 fTex;

out vec4 ofColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 cameraPos;
uniform Material material;
uniform Light light;

void main()
{
    vec3 norm = normalize(fNormal);
    vec3 lightDir = normalize(light.position - fPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 cameraDir = normalize(cameraPos - fPos);

    vec3 ambientLighting = light.ambient * vec3(texture(material.diffuse, fTex));

    float diffuseStrength = max(dot(lightDir, norm), 0.0f);
    vec3 diffuseLighting = light.diffuse * diffuseStrength * vec3(texture(material.diffuse, fTex));

    float specularStrength = pow(max(dot(reflectDir, cameraDir), 0.0f), material.shininess);
    vec3 specularLighting = light.specular * specularStrength * vec3(texture(material.specular, fTex));

    vec3 resColor = ambientLighting + diffuseLighting + specularLighting;
    ofColor = vec4(resColor, 1.0f);
}
