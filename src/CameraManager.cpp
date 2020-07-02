#include "CameraManager.hpp"
#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

std::weak_ptr<Camera> CameraManager::currentCamera;
GLFWwindow* CameraManager::currentWindow = nullptr;
std::vector<GLuint> CameraManager::framebuffers;
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
int CameraManager::viewportH = 720;
int CameraManager::viewportW = CameraManager::viewportH * CameraManager::aspectRatio;

void CameraManager::setActiveCamera(const std::shared_ptr<Camera>& newCamera){
    CameraManager::currentCamera = newCamera;
}

void CameraManager::setActiveWindow(GLFWwindow *window){
    CameraManager::removeMouseMovementCallback();
    CameraManager::removeViewportResizeCallback();
    CameraManager::currentWindow = window;
}

bool resizeFrameBufferAttachment(GLenum attachment, int newWidth, int newHeight){
    GLint boundObjectType = GL_NONE;
    GLint boundObjectName = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &boundObjectType);
    if (!(boundObjectType == GL_TEXTURE or boundObjectType == GL_RENDERBUFFER))
        return false;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &boundObjectName);
    if (boundObjectType == GL_TEXTURE){
        GLint currentTexture = 0;
        GLint currentTextureMultisample = 0;
        GLint currentTextureArray = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
        glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &currentTextureMultisample);
        glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &currentTextureArray);

        GLenum bindTarget = GL_TEXTURE_2D;
        glGetError();
        glBindTexture(bindTarget, boundObjectName);
        if (glGetError() == GL_INVALID_OPERATION){
            bindTarget = GL_TEXTURE_2D_MULTISAMPLE;
            glBindTexture(bindTarget, boundObjectName);
            if (glGetError() == GL_INVALID_OPERATION){
                bindTarget = GL_TEXTURE_2D_ARRAY;
                glBindTexture(bindTarget, boundObjectName);
                if (glGetError() == GL_INVALID_OPERATION){
                    glBindTexture(GL_TEXTURE_2D, currentTexture);
                    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, currentTextureMultisample);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, currentTextureArray);
                    return false;
                }
            }
        }
        GLint boundTextureFormat = GL_RGB;
        glGetTexLevelParameteriv(bindTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &boundTextureFormat);
        if (bindTarget == GL_TEXTURE_2D){
            glTexImage2D(GL_TEXTURE_2D, 0, boundTextureFormat, newWidth, newHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
        else if (bindTarget == GL_TEXTURE_2D_MULTISAMPLE){
            GLint boundTextureSamples = 1;
            glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_SAMPLES, &boundTextureSamples);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, boundTextureSamples, boundTextureFormat, newWidth, newHeight, GL_TRUE);
        }
        else if (bindTarget == GL_TEXTURE_2D_ARRAY){
            GLint boundTextureLayers = 1;
            glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_DEPTH, &boundTextureLayers);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, boundTextureFormat, newWidth, newHeight, boundTextureLayers, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, currentTextureMultisample);
        glBindTexture(GL_TEXTURE_2D_ARRAY, currentTextureArray);
    }
    else if (boundObjectType == GL_RENDERBUFFER){
        GLint currentRenderbuffer = 0;
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &currentRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, boundObjectName);
        GLint boundRenderBufferSamples = 1;
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &boundRenderBufferSamples);
        GLint boundRenderBufferFormat = GL_DEPTH24_STENCIL8;
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &boundRenderBufferFormat);
        if (boundRenderBufferSamples > 1){
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, boundRenderBufferSamples, boundRenderBufferFormat, newWidth, newHeight);
        }
        else {
            glRenderbufferStorage(GL_RENDERBUFFER, boundRenderBufferFormat, newWidth, newHeight);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, currentRenderbuffer);
    }
    return true;
}

void CameraManager::setViewportSize(int width, int height){
    CameraManager::aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    CameraManager::setVerticalFOV(CameraManager::verticalFOV);
    CameraManager::viewportW = width;
    CameraManager::viewportH = height;

    if (currentWindow){
        glViewport(0, 0, width, height);
    }

    GLint currentFrameBuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFrameBuffer);
    GLint maxColorAttachments = 8;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    for (const auto& framebuffer: framebuffers){
        if (framebuffer){
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            for (int i = 0; i < maxColorAttachments; i++){
                if (!resizeFrameBufferAttachment(GL_COLOR_ATTACHMENT0 + i, width, height))
                    break;
            }
            resizeFrameBufferAttachment(GL_DEPTH_STENCIL_ATTACHMENT, width, height);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, currentFrameBuffer);

    CameraManager::bStartup = true;
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
    CameraManager::setViewportSize(width, height);
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

std::pair<int, int> CameraManager::getViewportSize(){
    return {CameraManager::viewportW, CameraManager::viewportH};
}
