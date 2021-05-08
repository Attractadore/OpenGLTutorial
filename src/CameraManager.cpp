#include "CameraManager.hpp"
#include "Camera.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

std::weak_ptr<Camera> CameraManager::currentCamera;
GLFWwindow* CameraManager::currentWindow = nullptr;
float CameraManager::mouseX = 0.0f;
float CameraManager::mouseY = 0.0f;
float CameraManager::mouseSensitivityX = 0.05f;
float CameraManager::mouseSensitivityY = 0.05f;
bool CameraManager::bInvertMouseX = false;
bool CameraManager::bInvertMouseY = false;
bool CameraManager::bStartup = true;
float CameraManager::aspectRatio = 16.0f / 9.0f;
float CameraManager::horizontalFOV = 90.0f;
float CameraManager::verticalFOV = CameraManager::horizontalFOV / aspectRatio;
float CameraManager::nearPlane = 0.1f;
float CameraManager::farPlane = 100.0f;
glm::mat4 CameraManager::view;
glm::mat4 CameraManager::projection;

void CameraManager::setHorizontalFOV(float horizontalFOV) {
    CameraManager::horizontalFOV = horizontalFOV;
    CameraManager::verticalFOV = horizontalFOV / CameraManager::aspectRatio;
    CameraManager::updateProjectionMatrix();
}

void CameraManager::setVerticalFOV(float verticalFOV) {
    CameraManager::horizontalFOV = verticalFOV * CameraManager::aspectRatio;
    CameraManager::verticalFOV = verticalFOV;
    CameraManager::updateProjectionMatrix();
}

void CameraManager::setNearPlane(float nearPlane) {
    CameraManager::nearPlane = nearPlane;
    CameraManager::updateProjectionMatrix();
}

void CameraManager::setFarPlane(float farPlane) {
    CameraManager::farPlane = farPlane;
    CameraManager::updateProjectionMatrix();
}

void CameraManager::updateViewMatrix() {
    if (CameraManager::currentCamera.expired()) {
        return;
    }
    auto currentCameraPointer = CameraManager::currentCamera.lock();
    auto cameraPos = currentCameraPointer->cameraPos;
    auto cameraForwardVector = currentCameraPointer->getCameraForwardVector();
    auto cameraUpVector = currentCameraPointer->getCameraUpVector();
    CameraManager::view = glm::lookAt(cameraPos, cameraPos + cameraForwardVector, cameraUpVector);
}

void CameraManager::updateProjectionMatrix() {
    CameraManager::projection = glm::perspective(glm::radians(CameraManager::verticalFOV), CameraManager::aspectRatio, CameraManager::nearPlane, CameraManager::farPlane);
}

void CameraManager::activateMouseMovementCallback() {
    if (!CameraManager::currentWindow) {
        return;
    }
    glfwSetCursorPosCallback(CameraManager::currentWindow, CameraManager::mouseMovementCallback);
    bStartup = true;
}

void CameraManager::removeMouseMovementCallback() {
    if (!CameraManager::currentWindow) {
        return;
    }
    glfwSetCursorPosCallback(CameraManager::currentWindow, nullptr);
}

void CameraManager::mouseMovementCallback(GLFWwindow* window, double mouseX, double mouseY) {
    float dMouseX = mouseX - CameraManager::mouseX;
    if (bInvertMouseX)
        dMouseX *= -1;
    float dMouseY = mouseY - CameraManager::mouseY;
    if (bInvertMouseY)
        dMouseY *= -1;

    CameraManager::mouseX = mouseX;
    CameraManager::mouseY = mouseY;

    if (bStartup) {
        bStartup = false;
        return;
    }

    CameraManager::addCameraYawInput(-1.0f * dMouseX * CameraManager::mouseSensitivityX);
    CameraManager::addCameraPitchInput(-1.0f * dMouseY * CameraManager::mouseSensitivityY);
}

void CameraManager::addCameraYawInput(float degrees) {
    if (CameraManager::currentCamera.expired()) {
        return;
    }
    CameraManager::currentCamera.lock()->addYaw(degrees);
    CameraManager::updateViewMatrix();
}

void CameraManager::addCameraPitchInput(float degrees) {
    if (CameraManager::currentCamera.expired()) {
        return;
    }
    CameraManager::currentCamera.lock()->addPitch(degrees);
    CameraManager::updateViewMatrix();
}

void CameraManager::enableCameraLook() {
    if (!CameraManager::currentWindow) {
        return;
    }
    glfwSetInputMode(CameraManager::currentWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    CameraManager::activateMouseMovementCallback();
}

void CameraManager::disableCameraLook() {
    if (!CameraManager::currentWindow) {
        return;
    }
    glfwSetInputMode(CameraManager::currentWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    CameraManager::removeMouseMovementCallback();
}

void CameraManager::initialize(int viewportW, int viewportH) {
    CameraManager::aspectRatio = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    CameraManager::setVerticalFOV(CameraManager::horizontalFOV / CameraManager::aspectRatio);
}

void CameraManager::terminate() {
    CameraManager::currentCamera.reset();
    CameraManager::currentWindow = nullptr;
}

void CameraManager::processEvents() {
    if (currentWindow == nullptr) {
        return;
    }
    if (glfwGetKey(currentWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        CameraManager::disableCameraLook();
    }
    if (glfwGetMouseButton(currentWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        CameraManager::enableCameraLook();
    }
}

glm::vec3 CameraManager::getCameraMovementInput() {
    glm::vec3 cameraMovementInput = glm::vec3(0.0f);
    if (CameraManager::currentWindow != nullptr and !CameraManager::currentCamera.expired()) {
        auto camera = CameraManager::currentCamera.lock();
        if (glfwGetKey(CameraManager::currentWindow, GLFW_KEY_W) == GLFW_PRESS) {
            cameraMovementInput += camera->getCameraForwardVector();
        }
        if (glfwGetKey(CameraManager::currentWindow, GLFW_KEY_S) == GLFW_PRESS) {
            cameraMovementInput -= camera->getCameraForwardVector();
        }
        if (glfwGetKey(CameraManager::currentWindow, GLFW_KEY_D) == GLFW_PRESS) {
            cameraMovementInput += camera->getCameraRightVector();
        }
        if (glfwGetKey(CameraManager::currentWindow, GLFW_KEY_A) == GLFW_PRESS) {
            cameraMovementInput -= camera->getCameraRightVector();
        }
        if (glfwGetKey(CameraManager::currentWindow, GLFW_KEY_F) == GLFW_PRESS) {
            cameraMovementInput += camera->getCameraUpVector();
        }
        if (glfwGetKey(CameraManager::currentWindow, GLFW_KEY_V) == GLFW_PRESS) {
            cameraMovementInput -= camera->getCameraUpVector();
        }
    }
    return cameraMovementInput;
}

GLFWwindow* CameraManager::getWindow() {
    return CameraManager::currentWindow;
}
