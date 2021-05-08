#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

Camera::Camera(glm::vec3 cameraPos, glm::vec3 cameraLookDirection, glm::vec3 worldUpDirection) {
    cameraLookDirection = glm::normalize(cameraLookDirection);
    worldUpDirection = glm::normalize(worldUpDirection);
    this->cameraPos = cameraPos;
    this->cameraDefaultUpVector = worldUpDirection;
    this->cameraDefaultRightVector = glm::normalize(glm::cross(cameraLookDirection, worldUpDirection));
    this->cameraDefaultForwardVector = glm::cross(this->cameraDefaultUpVector, this->cameraDefaultRightVector);

    this->addPitch(glm::degrees(glm::asin(glm::dot(worldUpDirection, cameraLookDirection))));

    this->updateCameraForwardVector();
    this->updateCameraRightVector();
    this->updateCameraUpVector();
}

glm::vec3 Camera::getCameraForwardVector() const {
    return this->cameraForwardVector;
}

glm::vec3 Camera::getCameraRightVector() const {
    return this->cameraRightVector;
}

glm::vec3 Camera::getCameraUpVector() const {
    return this->cameraUpVector;
}

void Camera::addPitch(float degrees) {
    this->pitch += degrees;
    this->pitch = glm::clamp(this->pitch, -80.0f, 80.0f);

    this->pitchRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(this->pitch), this->cameraDefaultRightVector);

    this->updateCameraForwardVector();
    this->updateCameraUpVector();
}

void Camera::addYaw(float degrees) {
    this->yaw += degrees;

    this->yawRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(this->yaw), this->cameraDefaultUpVector);

    this->updateCameraRightVector();
    this->updateCameraForwardVector();
    this->updateCameraUpVector();
}

void Camera::addLocationOffset(glm::vec3 offset) {
    this->cameraPos += offset;
}

void Camera::updateCameraForwardVector() {
    this->cameraForwardVector = this->yawRotationMatrix * this->pitchRotationMatrix * glm::vec4(this->cameraDefaultForwardVector, 0.0f);
}

void Camera::updateCameraRightVector() {
    this->cameraRightVector = this->yawRotationMatrix * glm::vec4(this->cameraDefaultRightVector, 0.0f);
}

void Camera::updateCameraUpVector() {
    this->cameraUpVector = this->yawRotationMatrix * this->pitchRotationMatrix * glm::vec4(this->cameraDefaultUpVector, 0.0f);
}
