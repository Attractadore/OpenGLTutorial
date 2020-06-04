#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <IL/il.h>

void debugFunction(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam​){
    if (severity​ != GL_DEBUG_SEVERITY_NOTIFICATION){
        std::cout << message​ << std::endl;
    }
}

std::string loadShaderSource(std::filesystem::path filePath){
    std::fstream is(filePath);
    if (!is){
        return "";
    }
    auto fileSize = std::filesystem::file_size(filePath);
    std::string fileContents(fileSize, ' ');
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

void loadTextureData(GLuint textureId, std::filesystem::path filePath){
    if (!std::filesystem::exists(filePath)){
        return;
    }
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    ILuint loadedImage;
    ilGenImages(1, &loadedImage);
    ilBindImage(loadedImage);
    ilLoadImage(filePath.c_str());
    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    std::vector<std::byte> imageData(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, imageData.data());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    ilDeleteImage(loadedImage);
}

void enableCameraLook(GLFWwindow* window){
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    CameraManager::activateMouseMovementCallback();
}

void disableCameraLook(GLFWwindow* window){
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    CameraManager::removeMouseMovementCallback();
}

GLuint createShader(GLenum shaderType, std::string shaderSource){
    const char * shaderSourceCStr = shaderSource.c_str();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSourceCStr, nullptr);
    glCompileShader(shader);
    return shader;
}

GLuint createProgram(std::vector<GLuint> shaders){
    GLuint program = glCreateProgram();
    for (auto& shader : shaders){
        glAttachShader(program, shader);
    }
    glLinkProgram(program);
    for (auto& shader : shaders){
        glDetachShader(program, shader);
    }
    return program;
}

int main(){
    std::string windowTitle = "OpenGL Tutorial";
    float horizontalFOV = 90.0f;
    float aspectRatio = 16.0f / 9.0f;
    int defaultWindowHeight = 720;
    int defaultWindowWidth = defaultWindowHeight * aspectRatio;
    float rotateSpeed = 90.0f;
    float cameraSpeed = 3.0f;

    glm::vec3 cameraStartPos = {5.0f, 5.0f, 3.0f};
    glm::vec3 cameraStartLookDirection = -cameraStartPos;
    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDirection, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 cubeModel;
    glm::mat4 lampModel;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat3 normal;

    GLfloat cubeAmbient = 0.1f;
    GLfloat cubeSpecular = 0.5f;
    GLfloat cubeShininess = 32.0f;
    glm::vec3 cubeColor = {1.0f, 0.5f, 0.31f};

    glm::vec3 lightPos = {0.0f, 0.0f, 3.0f};
    glm::vec3 lightColor = {1.0f, 1.0f, 0.8f};
    glm::vec3 lampScale = 0.2f * glm::vec3(1.0f, 1.0f, 1.0f);
    GLfloat lightDayDuration = 300.0f;
    GLfloat lightAngularVelocity = 360.0f / lightDayDuration;

    GLuint cubeVertexShader, cubeFragmentShader;
    GLuint cubeShaderProgram;
    GLuint cubeVertexVBO;
    GLuint cubeVertexEBO;
    GLuint cubeVAO;
    GLuint lampVertexShader, lampFragmentShader;
    GLuint lampShaderProgram;
    GLuint lampVAO;

    // Cube uniform locations

    GLuint modelMatrixUniformLocation, viewMatrixUniformLocation, projectionMatrixUniformLocation, normalMatrixUniformLocation;
    GLuint lightColorUniformLocation, lightPosUniformLocation, cameraPosUniformLocation;
    GLuint materialAmbientUniformLocation, materialSpecularUniformLocation, materialShininessUniformLocation, materialColorUniformLocation;

    // Lamp uniform locations

    GLuint lampModelMatrixUniformLocation, lampViewMatrixUniformLocation, lampProjectionMatrixUniformLocation;
    GLuint lampLightColorUniformLocation;

    std::vector<GLfloat> vertexData =
    {
          1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
          1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
          1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
          1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,

         -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
         -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
         -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
         -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,

         -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
          1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
          1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,

         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
          1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
          1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,
         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,

          1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
          1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,

          1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,
          1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,
         -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f,
         -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f,
    };

    std::vector<GLuint> vertexIndices =
    {
         0,  1,  2,
         2,  3,  0,
         4,  5,  6,
         6,  7,  4,
         8,  9, 10,
        10, 11,  8,
        12, 13, 14,
        14, 15, 12,
        16, 17, 18,
        18, 19, 16,
        20, 21, 22,
        22, 23, 20,
    };

    auto cubeVertexShaderSource = loadShaderSource("assets/shaders/triangle.vert");
    auto cubeFragmentShaderSource = loadShaderSource("assets/shaders/triangle.frag");
    auto lampVertexShaderSource = loadShaderSource("assets/shaders/lamp.vert");
    auto lampFragmentShaderSource = loadShaderSource("assets/shaders/lamp.frag");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
    CameraManager::setActiveCamera(camera);

    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    if (glIsEnabled(GL_DEBUG_OUTPUT)){
        glDebugMessageCallback(debugFunction, nullptr);
    }
    glEnable(GL_DEPTH_TEST);

    // Create cube shader program

    cubeVertexShader = createShader(GL_VERTEX_SHADER, cubeVertexShaderSource);
    cubeFragmentShader = createShader(GL_FRAGMENT_SHADER, cubeFragmentShaderSource);
    cubeShaderProgram = createProgram({cubeVertexShader, cubeFragmentShader});
    glDeleteShader(cubeVertexShader);
    glDeleteShader(cubeFragmentShader);

    // Get cube shader uniform locations

    modelMatrixUniformLocation = glGetUniformLocation(cubeShaderProgram, "model");
    viewMatrixUniformLocation = glGetUniformLocation(cubeShaderProgram, "view");
    projectionMatrixUniformLocation = glGetUniformLocation(cubeShaderProgram, "projection");
    normalMatrixUniformLocation = glGetUniformLocation(cubeShaderProgram, "normal");

    lightPosUniformLocation = glGetUniformLocation(cubeShaderProgram, "lightPos");
    lightColorUniformLocation = glGetUniformLocation(cubeShaderProgram, "lightColor");
    cameraPosUniformLocation = glGetUniformLocation(cubeShaderProgram, "cameraPos");
    materialAmbientUniformLocation = glGetUniformLocation(cubeShaderProgram, "material.ambient");
    materialSpecularUniformLocation = glGetUniformLocation(cubeShaderProgram, "material.specular");
    materialShininessUniformLocation = glGetUniformLocation(cubeShaderProgram, "material.shininess");
    materialColorUniformLocation = glGetUniformLocation(cubeShaderProgram, "material.color");

    // Create lamp shader program

    lampVertexShader = createShader(GL_VERTEX_SHADER, lampVertexShaderSource);
    lampFragmentShader = createShader(GL_FRAGMENT_SHADER, lampFragmentShaderSource);
    lampShaderProgram = createProgram({lampVertexShader, lampFragmentShader});
    glDeleteShader(lampVertexShader);
    glDeleteShader(lampFragmentShader);

    // Get lamp shader uniform locations

    lampModelMatrixUniformLocation = glGetUniformLocation(lampShaderProgram, "model");
    lampViewMatrixUniformLocation = glGetUniformLocation(lampShaderProgram, "view");
    lampProjectionMatrixUniformLocation = glGetUniformLocation(lampShaderProgram, "projection");

    lampLightColorUniformLocation = glGetUniformLocation(lampShaderProgram, "lightColor");

    // Setup cube data

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVertexVBO);
    glGenBuffers(1, &cubeVertexEBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof (GLfloat) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVertexEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);

    // Setup lamp data

    glGenVertexArrays(1, &lampVAO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVertexVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVertexEBO);

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

        auto cameraForward = camera->getCameraForwardVector();
        auto cameraRight = camera->getCameraRightVector();
        auto cameraUp = camera->getCameraUpVector();
        cameraForward.z = 0.0f;
        cameraRight.z = 0.0f;
        cameraUp.x = cameraUp.y = 0.0f;
        glm::vec3 inputVector = cameraForward * forwardAxisValue + cameraRight * rightAxisValue + cameraUp * upAxisValue;
        if (glm::length(inputVector) > 0.0f){
            camera->addLocationOffset(glm::normalize(inputVector) * deltaTime * cameraSpeed);
        }

        cubeModel = glm::rotate(glm::mat4(1.0f), glm::radians(currentTime * rotateSpeed), {0.0f, 0.0f, 1.0f});
        view = CameraManager::getViewMatrix();
        projection = CameraManager::getProjectionMatrix();
        normal = glm::transpose(glm::inverse(glm::mat3(cubeModel)));

        glm::mat4 lightScale = glm::scale(glm::mat4(1.0f), lampScale);
        glm::mat4 lightRotate = glm::rotate(glm::mat4(1.0f), glm::radians(currentTime * lightAngularVelocity), {1.0f, 0.0f, 0.0f});
        glm::mat4 lightTranslate = glm::translate(glm::mat4(1.0f), lightPos);

        lampModel = lightRotate * lightTranslate * lightScale;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw cube

        glUseProgram(cubeShaderProgram);

        glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(cubeModel));
        glUniformMatrix4fv(viewMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix3fv(normalMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(normal));

        glUniform3fv(lightColorUniformLocation, 1, glm::value_ptr(lightColor));
        glUniform3fv(lightPosUniformLocation, 1, glm::value_ptr(lightRotate * glm::vec4(lightPos, 1.0f)));
        glUniform3fv(cameraPosUniformLocation, 1, glm::value_ptr(camera->getCameraPos()));
        glUniform1f(materialAmbientUniformLocation, cubeAmbient);
        glUniform1f(materialSpecularUniformLocation, cubeSpecular);
        glUniform1f(materialShininessUniformLocation, cubeShininess);
        glUniform3fv(materialColorUniformLocation, 1, glm::value_ptr(cubeColor));

        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

        // Draw lamp

        glUseProgram(lampShaderProgram);

        glUniformMatrix4fv(lampModelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(lampModel));
        glUniformMatrix4fv(lampViewMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lampProjectionMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(lampLightColorUniformLocation, 1, glm::value_ptr(lightColor));

        glBindVertexArray(lampVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();

        previousTime = currentTime;
    }

    CameraManager::setActiveCamera(nullptr);
    CameraManager::setActiveWindow(nullptr);

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &cubeVertexVBO);
    glDeleteBuffers(1, &cubeVertexEBO);
    glDeleteProgram(cubeShaderProgram);
    glDeleteProgram(lampShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
