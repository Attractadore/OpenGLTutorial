#pragma once
#include <glm/vec3.hpp>

#include <vector>

struct LightCommon {
    LightCommon();

    virtual void genColor();

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct DirectionalLight: virtual LightCommon {
    DirectionalLight();

    void genDirection();

    glm::vec3 direction;
};

struct PointLight: virtual LightCommon {
    PointLight();

    void genPosition();
    void genRadius();
    virtual void genColor() override;

    glm::vec3 position;
    float radius;
};

struct SpotLight: PointLight, DirectionalLight {
    SpotLight();

    void genCos();

    float innerAngleCos;
    float outerAngleCos;
};
