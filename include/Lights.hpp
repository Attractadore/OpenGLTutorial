#pragma once
#include <glm/glm.hpp>

#include <vector>

struct LightCommon {
    LightCommon();

    void genColor();

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

};

struct DirectionalLight : virtual LightCommon{
    DirectionalLight();

    void genDirection();

    glm::vec3 direction;
};

struct PointLight : virtual LightCommon {
    PointLight();

    void genPosition();
    void genK();

    glm::vec3 position;
    float kc = 1.0f;
    float kl, kq;
};

struct SpotLight : PointLight, DirectionalLight {
    SpotLight();

    void genCos();

    float innerAngleCos;
    float outerAngleCos;
};

