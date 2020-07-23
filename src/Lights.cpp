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

float sampleAmplification() {
    float aMin = 20.0f;
    float aMax = 50.0f;
    return RandomSampler::randomFloat(aMin, aMax);
}

float sampleAmbient() {
    return RandomSampler::randomFloat(0.05f, 0.2f);
}

float sampleRadius() {
    return RandomSampler::randomFloat(0.1f, 0.3f);
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

void PointLight::genRadius() {
    this->radius = sampleRadius();
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

void PointLight::genColor() {
    LightCommon::genColor();
    auto amp = sampleAmplification();
    this->ambient *= amp;
    this->diffuse *= amp;
    this->specular *= amp;
}

PointLight::PointLight() {
    this->genColor();
    this->genPosition();
    this->genRadius();
}

SpotLight::SpotLight():
    PointLight(), DirectionalLight() {
    this->genCos();
    this->radius *= 2.0f;
}
