#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    Camera(glm::vec3 cameraPos, glm::vec3 cameraLookDirection, glm::vec3 worldUpDirection);

    glm::vec3 getCameraPos();
    glm::vec3 getCameraForwardVector();
    glm::vec3 getCameraRightVector();
    glm::vec3 getCameraUpVector();

    void addPitch(float degrees);
    void addYaw(float degrees);

    void addLocationOffset(glm::vec3 offset);

private:
    glm::vec3 cameraPos = {0.0f, 0.0f, 0.0f};
    glm::vec3 cameraDefaultForwardVector = {1.0f, 0.0f, 0.0f};
    glm::vec3 cameraDefaultRightVector = {0.0f, -1.0f, 0.0f};
    glm::vec3 cameraDefaultUpVector = {0.0f, 0.0f, 1.0f};

    glm::mat4 pitchRotationMatrix = glm::mat4(1.0f);
    glm::mat4 yawRotationMatrix = glm::mat4(1.0f);

    glm::vec3 cameraForwardVector;
    glm::vec3 cameraRightVector;
    glm::vec3 cameraUpVector;

    float pitch = 0.0f;
    float yaw = 0.0f;

    void updateCameraForwardVector();
    void updateCameraRightVector();
    void updateCameraUpVector();
};
