#pragma once
#include <glm/glm.hpp>

#include <vector>

struct LightCommon {
    LightCommon();

    virtual void genColor();

    float ambient;
    glm::vec3 diffuse;
    float specular;

};

struct DirectionalLight : LightCommon{
    DirectionalLight();

    void genDirection();
    virtual void genColor() override;

    glm::vec3 direction;
    float distance;
};

struct PointLight : LightCommon {
    PointLight();

    void genPosition();
    virtual void genRadius();

    glm::vec3 position;
    float minRadius;
    float maxRadius;
};

struct SpotLight : PointLight{
    SpotLight();

    void genCos();
    virtual void genRadius() override;
    void genDirection();

    glm::vec3 direction;
    float innerAngleCos;
    float outerAngleCos;
};

