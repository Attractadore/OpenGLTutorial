#pragma once

#include <glm/mat4x4.hpp>

#include <memory>

class Camera;
class GLFWwindow;

class CameraManager {
public:
    CameraManager() = delete;
    CameraManager(const CameraManager& other) = delete;
    CameraManager(CameraManager&& other) = delete;

    static void setHorizontalFOV(float horizontalFOV);
    static void setVerticalFOV(float verticalFOV);
    static void setNearPlane(float nearPlane);
    static void setFarPlane(float farPlane);

    static const glm::mat4& getViewMatrix() {
        CameraManager::updateViewMatrix();
        return CameraManager::view;
    }
    static const glm::mat4& getProjectionMatrix() {
        return CameraManager::projection;
    }
    static float getHorizontalFOV() {
        return CameraManager::horizontalFOV;
    }
    static float getVerticalFOV() {
        return CameraManager::verticalFOV;
    }
    static float getNearPlane() {
        return CameraManager::nearPlane;
    }
    static float getFarPlane() {
        return CameraManager::farPlane;
    }
    static float getAspectRatio() {
        return CameraManager::aspectRatio;
    }

    static void addCameraPitchInput(float degrees);
    static void addCameraYawInput(float degrees);

    static void enableCameraLook();
    static void disableCameraLook();

    static void initialize(int viewportW, int viewportH);
    static void terminate();

    static void processEvents();
    static glm::vec3 getCameraMovementInput();

    static GLFWwindow* getWindow();

    static bool bInvertMouseX;
    static bool bInvertMouseY;
    static float mouseSensitivityX;
    static float mouseSensitivityY;
    static std::weak_ptr<Camera> currentCamera;

private:
    static float horizontalFOV;
    static float verticalFOV;
    static float nearPlane;
    static float farPlane;
    static glm::mat4 view;
    static glm::mat4 projection;

    static GLFWwindow* currentWindow;
    static float mouseX;
    static float mouseY;
    static bool bStartup;
    static float aspectRatio;

    static void mouseMovementCallback(GLFWwindow* window, double mouseX, double mouseY);
    static void activateMouseMovementCallback();
    static void removeMouseMovementCallback();

    static void updateProjectionMatrix();
    static void updateViewMatrix();
};
