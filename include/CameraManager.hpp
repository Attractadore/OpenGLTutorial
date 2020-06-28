#pragma once
#include "glad.h"

#include <glm/mat4x4.hpp>

#include <memory>
#include <vector>

class Camera;
class GLFWwindow;

class CameraManager {
public:
    CameraManager() = delete;
    CameraManager(const CameraManager& other) = delete;
    CameraManager(CameraManager&& other) = delete;

    static void activateMouseMovementCallback();
    static void removeMouseMovementCallback();
    static void activateViewportResizeCallback();
    static void removeViewportResizeCallback();

    static void setActiveCamera(const std::shared_ptr<Camera>& newCamera);
    static void setActiveWindow(GLFWwindow* window);
    static void setViewportSize(int width, int height);
    static void setHorizontalFOV(float horizontalFOV);
    static std::vector<GLuint> framebuffers;
    static void setVerticalFOV(float verticalFOV);
    static void setNearPlane(float nearPlane);
    static void setFarPlane(float farPlane);

    static void addCameraPitchInput(float degrees);
    static void addCameraYawInput(float degrees);

    static glm::mat4 getViewMatrix();
    static glm::mat4 getProjectionMatrix();
    static std::pair<int, int> getViewportSize();

private:
    static std::weak_ptr<Camera> currentCamera;
    static GLFWwindow* currentWindow;
    static float mouseX;
    static float mouseY;
    static float mouseSensitivityX;
    static float mouseSensitivityY;
    static bool bInvertMouseX;
    static bool bInvertMouseY;
    static bool bStartup;
    static float aspectRatio;
    static float horizontalFOV;
    static float verticalFOV;
    static float nearPlane;
    static float farPlane;
    static int viewportW;
    static int viewportH;

    static void mouseMovementCallback(GLFWwindow* window, double mouseX, double mouseY);
    static void viewportResizeCallback(GLFWwindow* window, int width, int height);
};
