#include "Lights.hpp"
#include "RandomSampler.hpp"

#include <glm/glm.hpp>

#include <tuple>

glm::vec3 sampleVector(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax) {
    return {RandomSampler::randomFloat(xMin, xMax), RandomSampler::randomFloat(yMin, yMax), RandomSampler::randomFloat(zMin, zMax)};
}

glm::vec3 sampleDirection() {
    return glm::normalize(sampleVector(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 0.0f));
}

glm::vec3 samplePosition() {
    float xMin, xMax, yMax, yMin, zMin, zMax;
    yMax = xMax = 20.f;
    yMin = xMin = -xMax;
    zMin = 2.0f;
    zMax = 5.0f;
    return sampleVector(xMin, xMax, yMin, yMax, zMin, zMax);
}

glm::vec3 sampleLightColor() {
    float cMin = 0.3f;
    float cMax = 1.0f;
    return sampleVector(cMin, cMax, cMin, cMax, cMin, cMax);
}

float sampleAmbient() {
    return RandomSampler::randomFloat(0.05f, 0.2f);
}

std::tuple<float, float, float> sampleK() {
    return {1.0f, RandomSampler::randomFloat(0.0f, 0.05f), RandomSampler::randomFloat(0.0f, 0.05f)};
}

std::pair<float, float> sampleCos() {
    float outerMin, innerMin;
    outerMin = 0.5f;
    innerMin = 0.85f;
    return {RandomSampler::randomFloat(innerMin, 1.0f), RandomSampler::randomFloat(outerMin, innerMin)};
}

void LightCommon::genColor() {
    this->specular = this->diffuse = sampleLightColor();
    this->ambient = this->diffuse * sampleAmbient();
}

void DirectionalLight::genDirection() {
    this->direction = sampleDirection();
}

void PointLight::genPosition() {
    this->position = samplePosition();
}

void PointLight::genK() {
    std::tie(this->kc, this->kl, this->kq) = sampleK();
}

void SpotLight::genCos() {
    std::tie(this->innerAngleCos, this->outerAngleCos) = sampleCos();
}

LightCommon::LightCommon() {
    this->genColor();
}

DirectionalLight::DirectionalLight():
    LightCommon() {
    this->genDirection();
}

PointLight::PointLight():
    LightCommon() {
    this->genPosition();
    this->genK();
}

SpotLight::SpotLight():
    PointLight(), DirectionalLight() {
    this->genCos();
}
