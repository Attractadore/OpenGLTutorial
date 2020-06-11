#include "Lights.hpp"

#include "RandomSampler.hpp"

#include <tuple>

glm::vec3 sampleVector(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax){
    return {RandomSampler::randomFloat(xMin, xMax), RandomSampler::randomFloat(yMin, yMax), RandomSampler::randomFloat(zMin, zMax)};
}

glm::vec3 sampleDirection(){
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

glm::vec3 sampleLightLuminousIntencity(){
    float cMin = 50.0f;
    float cMax = 100.0f;
    return sampleVector(cMin, cMax, cMin, cMax, cMin, cMax);
}

glm::vec3 sampleDirLightLuminousIntencity(){
    float cMin = 0.1f;
    float cMax = 0.75f;
    return sampleVector(cMin, cMax, cMin, cMax, cMin, cMax);
}

float sampleAmbient(){
    return RandomSampler::randomFloat(0.1f, 0.2f);
}

float sampleSpecular(){
    return RandomSampler::randomFloat(1.0f, 3.0f);
}

std::pair<float, float> sampleCos(){
    float outerMin, innerMin;
    outerMin = 0.5f;
    innerMin = 0.85f;
    return {RandomSampler::randomFloat(innerMin, 1.0f), RandomSampler::randomFloat(outerMin, innerMin)};
}

float samplePointLightMinRadius(){
    return RandomSampler::randomFloat(0.2f, 0.4f);
}

float sampleSpotLightMinRadius(){
    return RandomSampler::randomFloat(0.4, 0.6f);
}

void LightCommon::genColor(){
    this->ambient = sampleAmbient();
    this->diffuse = sampleLightLuminousIntencity();
    this->specular = sampleSpecular();
}

void DirectionalLight::genDirection(){
    this->direction = sampleDirection();
}

void DirectionalLight::genColor(){
    this->ambient = sampleAmbient();
    this->diffuse = sampleDirLightLuminousIntencity();
    this->distance = 1.0f;
    this->specular = sampleSpecular();
}

void PointLight::genPosition(){
    this->position = samplePosition();
}

void PointLight::genRadius(){
    this->minRadius = samplePointLightMinRadius();
    this->maxRadius = 100.0f;
}

void SpotLight::genCos(){
    std::tie(this->innerAngleCos, this->outerAngleCos) = sampleCos();
}

void SpotLight::genRadius(){
    this->minRadius = sampleSpotLightMinRadius();
    this->maxRadius = 100.0f;
}

void SpotLight::genDirection(){
    this->direction = sampleDirection();
}

LightCommon::LightCommon(){
    this->genColor();
}

DirectionalLight::DirectionalLight(){
    this->genColor();
    this->genDirection();
}

PointLight::PointLight() : LightCommon(){
    this->genPosition();
    this->genRadius();
}

SpotLight::SpotLight() : PointLight(){
    this->genDirection();
    this->genRadius();
    this->genCos();
}
