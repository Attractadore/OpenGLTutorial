#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <IL/il.h>

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>

void debugMessageCallback(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam){
    if (severity​ != GL_DEBUG_SEVERITY_NOTIFICATION){
        std::cout << message​ << std::endl;
    }
}

std::vector<char> getBinary(std::filesystem::path filePath){
    std::ifstream is(filePath, std::ios::binary);
    if (!is){
        return {};
    }
    auto fileSize = std::filesystem::file_size(filePath);
    std::vector<char> fileContents(fileSize);
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

void loadTexture(GLuint textureID, std::filesystem::path filePath){
    if (!std::filesystem::exists(filePath)){
        return;
    }
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    ILuint loadImage;
    ilGenImages(1, &loadImage);
    ilBindImage(loadImage);
    ilLoadImage(filePath.c_str());
    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    int levels = std::log2(std::min(width, height)) + 1;
    glTextureStorage2D(textureID, levels, GL_RGBA8, width, height);
    std::vector<std::byte> imageData(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, imageData.data());
    glTextureSubImage2D(textureID, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
    glGenerateTextureMipmap(textureID);
    ilDeleteImage(loadImage);
}

void enableCameraLook(GLFWwindow* window){
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    CameraManager::activateMouseMovementCallback();
}

void disableCameraLook(GLFWwindow* window){
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    CameraManager::removeMouseMovementCallback();
}

int main(){
    std::string windowTitle = "OpenGL Tutorial";
    float horizontalFOV = 90.0f;
    float aspectRatio = 16.0f / 9.0f;
    int defaultWindowHeight = 720;
    int defaultWindowWidth = defaultWindowHeight * aspectRatio;
    float rotateSpeed = 90.0f;
    float cameraSpeed = 3.0f;

    Camera camera({-10.0f, 0.0f, 5.0f}, {10.0f, 0.0f, -5.0f}, {0.0f, 0.0f, 1.0f});
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    GLuint vertShader;
    GLuint fragShader;
    GLuint triangleShaderProgram;
    GLuint triangleVertexVBO;
    GLuint triangleVertexEBO;
    GLuint triangleVAO;
    GLuint triangleBackgroundTexture;
    GLuint triangleForegroundTexture;
    GLuint planeVAO;
    GLuint planeVertexVBO;
    GLuint planeVertexEBO;

    std::vector<GLfloat> triangleVertexData =
    {
         1.0f,  1.0f,  1.0f, 1.0f, 0.f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f, 1.f, 0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 0.0f, 0.f, 1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, 1.0f, 1.f, 1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, -1.0f, 0.0f, 0.f, 1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f, 1.f, 1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f, 1.0f, 0.f, 0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 0.0f, 1.f, 0.0f, 1.0f, 1.0f,
    };

    std::vector<GLuint> triangleVertexIndices =
    {
        0, 1, 2,
        2, 3, 0,
        0, 1, 4,
        1, 4, 5,
        1, 2, 6,
        1, 5, 6,
        2, 3, 7,
        2, 6, 7,
        0, 3, 7,
        0, 4, 7,
        4, 5, 6,
        4, 7, 6,
    };

    std::vector<GLfloat> planeVertexData = {
         10.0f,  10.0f, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f, 1.0f,
         10.0f, -10.0f, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f, 1.0f,
        -10.0f, -10.0f, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f, 1.0f,
        -10.0f,  10.0f, 0.0f, 0.2f, 0.2f, 0.2f, 1.0f, 1.0f,
    };

    std::vector<GLuint> planeVertexIndices = {
        0, 1, 2,
        2, 3, 0,
    };

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(defaultWindowWidth, defaultWindowHeight, windowTitle.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    CameraManager::setActiveWindow(window);
    CameraManager::activateViewportResizeCallback();
    glfwSetWindowSize(window, defaultWindowWidth, defaultWindowHeight);
    enableCameraLook(window);
    CameraManager::setViewportSize(defaultWindowWidth, defaultWindowHeight);
    CameraManager::setHorizontalFOV(horizontalFOV);
    CameraManager::setActiveCamera(&camera);

    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    if (glIsEnabled(GL_DEBUG_OUTPUT)){
        glDebugMessageCallback(debugMessageCallback, nullptr);
    }
    glEnable(GL_DEPTH_TEST);

    vertShader = glCreateShader(GL_VERTEX_SHADER);
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    auto vertShaderBinary = getBinary("assets/shaders/bin/triangle.vert");
    auto fragShaderBinary = getBinary("assets/shaders/bin/triangle.frag");

    glShaderBinary(1, &vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertShaderBinary.data(), vertShaderBinary.size());
    glSpecializeShader(vertShader, "main", 0, nullptr, nullptr);
    glShaderBinary(1, &fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragShaderBinary.data(), fragShaderBinary.size());
    glSpecializeShader(fragShader, "main", 0, nullptr, nullptr);
    triangleShaderProgram = glCreateProgram();
    glAttachShader(triangleShaderProgram, vertShader);
    glAttachShader(triangleShaderProgram, fragShader);
    glLinkProgram(triangleShaderProgram);
    glDetachShader(triangleShaderProgram, vertShader);
    glDetachShader(triangleShaderProgram, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    glCreateVertexArrays(1, &triangleVAO);
    glCreateBuffers(1, &triangleVertexVBO);
    glCreateBuffers(1, &triangleVertexEBO);

    glNamedBufferData(triangleVertexVBO, sizeof (GLfloat) * triangleVertexData.size(), triangleVertexData.data(), GL_STATIC_DRAW);
    glVertexArrayAttribFormat(triangleVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(triangleVAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));
    glVertexArrayAttribFormat(triangleVAO, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat));
    glVertexArrayAttribBinding(triangleVAO, 0, 0);
    glVertexArrayAttribBinding(triangleVAO, 1, 0);
    glVertexArrayAttribBinding(triangleVAO, 2, 0);
    glVertexArrayVertexBuffer(triangleVAO, 0, triangleVertexVBO, 0, 8 * sizeof (GLfloat));
    glEnableVertexArrayAttrib(triangleVAO, 0);
    glEnableVertexArrayAttrib(triangleVAO, 1);
    glEnableVertexArrayAttrib(triangleVAO, 2);
    glNamedBufferData(triangleVertexEBO, sizeof (GLuint) * triangleVertexIndices.size(), triangleVertexIndices.data(), GL_STATIC_DRAW);
    glVertexArrayElementBuffer(triangleVAO, triangleVertexEBO);

    glCreateVertexArrays(1, &planeVAO);
    glCreateBuffers(1, &planeVertexVBO);
    glCreateBuffers(1, &planeVertexEBO);

    glNamedBufferData(planeVertexVBO, sizeof (GLfloat) * planeVertexData.size(), planeVertexData.data(), GL_STATIC_DRAW);
    glVertexArrayAttribFormat(planeVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(planeVAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));
    glVertexArrayAttribFormat(planeVAO, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat));
    glVertexArrayAttribBinding(planeVAO, 0, 0);
    glVertexArrayAttribBinding(planeVAO, 1, 0);
    glVertexArrayAttribBinding(planeVAO, 2, 0);
    glVertexArrayVertexBuffer(planeVAO, 0, planeVertexVBO, 0, 8 * sizeof (GLfloat));
    glEnableVertexArrayAttrib(planeVAO, 0);
    glEnableVertexArrayAttrib(planeVAO, 1);
    glEnableVertexArrayAttrib(planeVAO, 2);
    glNamedBufferData(planeVertexEBO, sizeof (GLuint) * planeVertexIndices.size(), planeVertexIndices.data(), GL_STATIC_DRAW);
    glVertexArrayElementBuffer(planeVAO, planeVertexEBO);

    glCreateTextures(GL_TEXTURE_2D, 1, &triangleBackgroundTexture);
    loadTexture(triangleBackgroundTexture, "assets/textures/tex1.png");
    glCreateTextures(GL_TEXTURE_2D, 1, &triangleForegroundTexture);
    loadTexture(triangleForegroundTexture, "assets/textures/tex2.png");

    float forwardAxisValue, rightAxisValue, upAxisValue;

    float previousTime = 0.0f;

    while (not glfwWindowShouldClose(window)){
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - previousTime;

        forwardAxisValue = 0.0f;
        rightAxisValue = 0.0f;
        upAxisValue = 0.0f;

        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
            disableCameraLook(window);
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
            enableCameraLook(window);
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
            forwardAxisValue += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
            forwardAxisValue -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
            rightAxisValue += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
            rightAxisValue -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS){
            upAxisValue -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS){
            upAxisValue += 1.0f;
        }

        auto cameraForward = camera.getCameraForwardVector();
        auto cameraRight = camera.getCameraRightVector();
        auto cameraUp = camera.getCameraUpVector();
        cameraForward.z = 0.0f;
        cameraRight.z = 0.0f;
        cameraUp.x = cameraUp.y = 0.0f;
        glm::vec3 inputVector = cameraForward * forwardAxisValue + cameraRight * rightAxisValue + cameraUp * upAxisValue;
        if (glm::length(inputVector) > 0.0f){
            camera.addLocationOffset(glm::normalize(inputVector) * deltaTime * cameraSpeed);
        }

        model = glm::rotate(glm::mat4(1.0f), glm::radians(currentTime * rotateSpeed), {0.0f, 0.0f, 1.0f});
        model = glm::translate(model, {0.0f, 0.0f, 1.0f});
        view = CameraManager::getViewMatrix();
        projection = CameraManager::getProjectionMatrix();

        glClearColor(0.1f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(triangleShaderProgram);
        glUniform1f(0, currentTime);
        glUniform1i(1, 0);
        glUniform1i(2, 1);
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(projection));
        glBindTextureUnit(0, triangleBackgroundTexture);
        glBindTextureUnit(1, triangleForegroundTexture);

        glBindVertexArray(triangleVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();

        previousTime = currentTime;
    }

    CameraManager::setActiveCamera(nullptr);
    CameraManager::setActiveWindow(nullptr);

    glDeleteTextures(1, &triangleBackgroundTexture);
    glDeleteTextures(1, &triangleForegroundTexture);
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVertexVBO);
    glDeleteBuffers(1, &triangleVertexEBO);
    glDeleteProgram(triangleShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
