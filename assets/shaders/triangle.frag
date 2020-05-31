#version 330 core
in vec3 fPos;
in vec3 fNormal;

out vec4 ofColor;

struct Material {
    float ambient, specular, shininess;
    vec3 color;
};

uniform vec3 lightPos, lightColor, cameraPos;
uniform Material material;

void main()
{
    vec3 norm = normalize(fNormal);
    vec3 lightDir = normalize(lightPos - fPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 cameraDir = normalize(cameraPos - fPos);

    float ambientStrength = material.ambient;
    float diffuseStrength = max(dot(lightDir, norm), 0.0f);
    float specularStrength =  material.specular * pow(max(dot(reflectDir, cameraDir), 0.0f), material.shininess);

    vec3 resLighting = (ambientStrength + diffuseStrength + specularStrength) * lightColor;
    vec3 resColor = material.color * resLighting;

    ofColor = vec4(resColor, 1.0f);
}