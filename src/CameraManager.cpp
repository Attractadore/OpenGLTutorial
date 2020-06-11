#include "CameraManager.hpp"
#include "Camera.hpp"

#include "glad.h"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

std::weak_ptr<Camera> CameraManager::currentCamera;
GLFWwindow* CameraManager::currentWindow = nullptr;
float CameraManager::mouseX = 0.0f;
float CameraManager::mouseY = 0.0f;
float CameraManager::mouseSensitivityX = 0.05f;
float CameraManager::mouseSensitivityY = 0.05f;
bool CameraManager::bInvertMouseX = false;
bool CameraManager::bInvertMouseY = false;
bool CameraManager::bStartup = true;
float CameraManager::aspectRatio = 4.0f / 3.0f;
float CameraManager::horizontalFOV = 90.0f;
float CameraManager::verticalFOV = CameraManager::horizontalFOV / aspectRatio;
float CameraManager::nearPlane = 0.1f;
float CameraManager::farPlane = 100.0f;

void CameraManager::setActiveCamera(std::shared_ptr<Camera> newCamera){
    CameraManager::currentCamera = newCamera;
}

void CameraManager::setActiveWindow(GLFWwindow *window){
    CameraManager::removeMouseMovementCallback();
    CameraManager::removeViewportResizeCallback();
    CameraManager::currentWindow = window;
}

void CameraManager::setViewportSize(int width, int height){
    CameraManager::aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    CameraManager::setVerticalFOV(CameraManager::verticalFOV);
}

void CameraManager::setHorizontalFOV(float horizontalFOV){
    CameraManager::horizontalFOV = horizontalFOV;
    CameraManager::verticalFOV = horizontalFOV / CameraManager::aspectRatio;
}

void CameraManager::setVerticalFOV(float verticalFOV){
    CameraManager::horizontalFOV = verticalFOV * CameraManager::aspectRatio;
    CameraManager::verticalFOV = verticalFOV;
}

void CameraManager::setNearPlane(float nearPlane){
    CameraManager::nearPlane = nearPlane;
}

void CameraManager::setFarPlane(float farPlane){
    CameraManager::farPlane = farPlane;
}

void CameraManager::activateMouseMovementCallback(){
    if (!CameraManager::currentWindow){
        return;
    }
    glfwSetCursorPosCallback(CameraManager::currentWindow, CameraManager::mouseMovementCallback);
    bStartup = true;
}

void CameraManager::removeMouseMovementCallback(){
    if (!CameraManager::currentWindow){
        return;
    }
    glfwSetCursorPosCallback(CameraManager::currentWindow, nullptr);
}

void CameraManager::activateViewportResizeCallback(){
    if (!CameraManager::currentWindow){
        return;
    }
    glfwSetFramebufferSizeCallback(CameraManager::currentWindow, CameraManager::viewportResizeCallback);
}

void CameraManager::removeViewportResizeCallback(){
    if (!CameraManager::currentWindow){
        return;
    }
    glfwSetFramebufferSizeCallback(CameraManager::currentWindow, nullptr);
}

void CameraManager::mouseMovementCallback(GLFWwindow *window, double mouseX, double mouseY){
    float dMouseX = mouseX - CameraManager::mouseX;
    if (bInvertMouseX) dMouseX *= -1;
    float dMouseY = mouseY - CameraManager::mouseY;
    if (bInvertMouseY) dMouseY *= -1;

    CameraManager::mouseX = mouseX;
    CameraManager::mouseY = mouseY;

    if (bStartup){
        bStartup = false;
        return;
    }

    CameraManager::addCameraYawInput(-1.0f * dMouseX * CameraManager::mouseSensitivityX);
    CameraManager::addCameraPitchInput(-1.0f * dMouseY * CameraManager::mouseSensitivityY);
}

void CameraManager::viewportResizeCallback(GLFWwindow *window, int width, int height){
    glViewport(0, 0, width, height);
    CameraManager::setViewportSize(width, height);
    CameraManager::bStartup = true;
}

void CameraManager::addCameraYawInput(float degrees){
    if (CameraManager::currentCamera.expired()){
        return;
    }
    CameraManager::currentCamera.lock()->addYaw(degrees);
}

void CameraManager::addCameraPitchInput(float degrees){
    if (CameraManager::currentCamera.expired()){
        return;
    }
    CameraManager::currentCamera.lock()->addPitch(degrees);
}

glm::mat4 CameraManager::getViewMatrix(){
    if (CameraManager::currentCamera.expired()){
        return glm::mat4(1.0f);
    }
    auto currentCameraPointer = CameraManager::currentCamera.lock();
    auto cameraPos = currentCameraPointer->getCameraPos();
    auto cameraForwardVector = currentCameraPointer->getCameraForwardVector();
    auto cameraUpVector = currentCameraPointer->getCameraUpVector();
    return glm::lookAt(cameraPos, cameraPos + cameraForwardVector, cameraUpVector);
}

glm::mat4 CameraManager::getProjectionMatrix(){
    return glm::perspective(glm::radians(CameraManager::verticalFOV), CameraManager::aspectRatio, CameraManager::nearPlane, CameraManager::farPlane);
}
