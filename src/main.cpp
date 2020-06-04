#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "Lights.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <IL/il.h>
#include <fmt/format.h>

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>
#include <type_traits>

#include <cmath>

void debugFunction(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam​){
    if (severity​ != GL_DEBUG_SEVERITY_NOTIFICATION){
        std::printf("%s\n", message​);
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
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

template <class T>
void setLightPosition(GLuint shaderProgram, std::string arrayName, const std::vector<T>& arrayData, int i){
    glUniform3fv(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].position", arrayName, i).c_str()), 1, glm::value_ptr(arrayData[i].position));
}

template <class T>
void setLightDirection(GLuint shaderProgram, std::string arrayName, const std::vector<T>& arrayData, int i){
    glUniform3fv(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].direction", arrayName, i).c_str()), 1, glm::value_ptr(arrayData[i].direction));
}

template <class T>
void setLightColor(GLuint shaderProgram, std::string arrayName, const std::vector<T>& arrayData, int i){
    glUniform3fv(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].color.ambient", arrayName, i).c_str()), 1, glm::value_ptr(arrayData[i].ambient));
    glUniform3fv(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].color.diffuse", arrayName, i).c_str()), 1, glm::value_ptr(arrayData[i].diffuse));
    glUniform3fv(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].color.specular", arrayName, i).c_str()), 1, glm::value_ptr(arrayData[i].specular));
}

template <class T>
void setLightK(GLuint shaderProgram, std::string arrayName, const std::vector<T>& arrayData, int i){
    glUniform1f(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].k.c", arrayName, i).c_str()), arrayData[i].kc);
    glUniform1f(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].k.l", arrayName, i).c_str()), arrayData[i].kl);
    glUniform1f(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].k.q", arrayName, i).c_str()), arrayData[i].kq);
}

void setLightCone(GLuint shaderProgram, std::string arrayName, const std::vector<SpotLight>& arrayData, int i){
    glUniform1f(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].inner", arrayName, i).c_str()), arrayData[i].innerAngleCos);
    glUniform1f(glGetUniformLocation(shaderProgram, fmt::format("{}[{}].outer", arrayName, i).c_str()), arrayData[i].outerAngleCos);
}

void setupPointLights(GLuint shaderProgram, const std::vector<PointLight>& pointLights){
    glUniform1i(glGetUniformLocation(shaderProgram, "numPointLights"), pointLights.size());
    std::string arrayName = "pointLights";
    for (int i = 0; i < pointLights.size(); i++){
        setLightPosition(shaderProgram, arrayName, pointLights, i);
        setLightColor(shaderProgram, arrayName, pointLights, i);
        setLightK(shaderProgram, arrayName, pointLights, i);
    }
}

void setupSpotLights(GLuint shaderProgram, const std::vector<SpotLight>& spotLights, bool bFlashLight) {
    int numSpotLights = std::max((int)(spotLights.size() - !bFlashLight), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "numSpotLights"), numSpotLights);
    std::string arrayName = "spotLights";
    for (int i = 0; i < numSpotLights; i++){
        setLightPosition(shaderProgram, arrayName, spotLights, i);
        setLightDirection(shaderProgram, arrayName, spotLights, i);
        setLightCone(shaderProgram, arrayName, spotLights, i);
        setLightColor(shaderProgram, arrayName, spotLights, i);
        setLightK(shaderProgram, arrayName, spotLights, i);
    }
}

void setupDirectionalLights(GLuint shaderProgram, const std::vector<DirectionalLight>& directionalLights){
    glUniform1i(glGetUniformLocation(shaderProgram, "numDirLights"), directionalLights.size());
    std::string arrayName = "dirLights";
    for (int i = 0; i < directionalLights.size(); i++){
        setLightDirection(shaderProgram, arrayName, directionalLights, i);
        setLightColor(shaderProgram, arrayName, directionalLights, i);
    }
}

template <typename T, typename V>
void storeData(const std::vector<T>& vertexData, const std::vector<V>& vertexIndices, GLuint VBO, GLuint EBO) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof (T) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (V) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);
}

void setupModel(GLuint VAO, GLuint VBO, GLuint EBO){
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void *) (3 * sizeof(GLfloat)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void *) (6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

}

void setupLamp(GLuint VAO, GLuint VBO, GLuint EBO){
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
}

void setCubeUniforms(GLuint shaderProgram, float x, float y){
    glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), {x, y, 0.0f});
    glm::mat3 cubeNormal = glm::transpose(glm::inverse(glm::mat3(cubeModel)));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normal"), 1, GL_FALSE, glm::value_ptr(cubeNormal));
}

void setPyramidUniforms(GLuint shaderProgram, float x, float y){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), {x, y, -1.0f});
    model = glm::scale(model, 2.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normal"), 1, GL_FALSE, glm::value_ptr(normal));
}

void setPointLightLampUniforms(GLuint shaderProgram, const PointLight& pl){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pl.position);
    model = glm::scale(model, 0.2f * glm::vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(pl.diffuse));
}

void setSpotLightLampUniforms(GLuint shaderProgram, const SpotLight& sl){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), sl.position);
    float angle = glm::acos(glm::dot(glm::normalize(sl.direction), {0.0f, 0.0f, -1.0f}));
    glm::vec3 rotateAxis = glm::normalize(glm::cross({0.0f, 0.0f, -1.0f}, sl.direction));
    model = glm::rotate(model, angle, rotateAxis);
    model = glm::scale(model, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(sl.diffuse));
}

void setDirectionalLightLampUniforms(GLuint shaderProgram, const DirectionalLight& dl){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 30.0f});
    float angle = glm::acos(glm::dot(glm::normalize(dl.direction), {0.0f, 0.0f, -1.0f}));
    glm::vec3 rotateAxis = glm::normalize(glm::cross({0.0f, 0.0f, -1.0f}, dl.direction));
    model = glm::rotate(model, angle, rotateAxis);
    model = glm::scale(model, 10.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(dl.diffuse));
}

int main(){
    std::string windowTitle = "OpenGL Tutorial";
    float horizontalFOV = 90.0f;
    float aspectRatio = 16.0f / 9.0f;
    int defaultWindowHeight = 720;
    int defaultWindowWidth = defaultWindowHeight * aspectRatio;
    float cameraSpeed = 5.0f;

    int numPointLights = 5;
    int numSpotLights = 5 + 1;
    int numDirectionalLights = 1;
    bool bFlashLight = false;

    int numCubesX = 10;
    int numCubesY = numCubesX;
    float cubeDistanceX = 4.0f;
    float cubeDistanceY = cubeDistanceX;

    std::vector<PointLight> pointLights(numPointLights);
    std::vector<SpotLight> spotLights(numSpotLights);
    std::vector<DirectionalLight> directionalLights(numDirectionalLights);

    glm::vec3 cameraStartPos = {5.0f, 5.0f, 3.0f};
    glm::vec3 cameraStartLookDirection = -cameraStartPos;
    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDirection, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 floorScale = 25.0f * glm::vec3(1.0f, 1.0f, 1.0f);

    glm::mat4 floorModel;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat3 floorNormal;

    std::filesystem::path cubeDiffuseMapPath = "assets/textures/container2.png";
    std::filesystem::path cubeSpecularMapPath = "assets/textures/container2_specular.png";
    GLfloat cubeShininess = 32.0f;

    GLuint cubeVertexShader, cubeFragmentShader;
    GLuint lampVertexShader, lampFragmentShader;
    GLuint cubeShaderProgram, lampShaderProgram;
    GLuint cubeDiffuseMap, cubeSpecularMap;

    std::vector<GLuint> vertexBuffers(3);
    std::vector<GLuint> elementBuffers(3);
    std::vector<GLuint> vertexArrays(6);
    GLuint& cubeVBO = vertexBuffers[0];
    GLuint& pyramidVBO = vertexBuffers[1];
    GLuint& planeVBO = vertexBuffers[2];
    GLuint& cubeEBO = elementBuffers[0];
    GLuint& pyramidEBO = elementBuffers[1];
    GLuint& planeEBO = elementBuffers[2];
    GLuint& cubeVAO = vertexArrays[0];
    GLuint& pyramidVAO = vertexArrays[1];
    GLuint& planeVAO = vertexArrays[2];
    GLuint& pointLightVAO = vertexArrays[3];
    GLuint& spotLightVAO = vertexArrays[4];
    GLuint& directionalLightVAO = vertexArrays[5];

    std::vector<GLfloat> cubeVertexData =
    {
          1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
          1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
          1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
          1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

         -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,

         -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
          1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
          1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
         -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,

         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
          1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
          1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

          1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
          1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,

          1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
          1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
    };

    std::vector<GLuint> cubeVertexIndices =
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

    std::vector<GLfloat> pyramidVertexData = {
                           0.0f,  1.0f, 0.0f, 0.0f, 0.0f, -1.0f,  std::sqrt(3.0f) * 0.25f, 0.75f,
         std::sqrt(3.0f) * 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f,   std::sqrt(3.0f) * 0.5f,  0.0f,
        -std::sqrt(3.0f) * 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f,                     0.0f,  0.0f,

                           0.0f,  1.0f,            0.0f, 0.25f * std::sqrt(3.0f), 0.25f, 0.25f / std::sqrt(2.0f), 1.0f,                   0.0f,
         std::sqrt(3.0f) * 0.5f, -0.5f,            0.0f, 0.25f * std::sqrt(3.0f), 0.25f, 0.25f / std::sqrt(2.0f), 0.0f,                   0.0f,
                           0.0f,  0.0f, std::sqrt(2.0f), 0.25f * std::sqrt(3.0f), 0.25f, 0.25f / std::sqrt(2.0f), 0.5f, std::sqrt(2.0f / 3.0f),

         std::sqrt(3.0f) * 0.5f, -0.5f,            0.0f, 0.0f, -0.5f, 0.25f / std::sqrt(2.0f), 1.0f,                   0.0f,
        -std::sqrt(3.0f) * 0.5f, -0.5f,            0.0f, 0.0f, -0.5f, 0.25f / std::sqrt(2.0f), 0.0f,                   0.0f,
                           0.0f,  0.0f, std::sqrt(2.0f), 0.0f, -0.5f, 0.25f / std::sqrt(2.0f), 0.5f, std::sqrt(2.0f / 3.0f),

                           0.0f,  1.0f,            0.0f, -0.25f * std::sqrt(3.0f), 0.25f, 0.25f / std::sqrt(2.0f), 0.0f,                   0.0f,
        -std::sqrt(3.0f) * 0.5f, -0.5f,            0.0f, -0.25f * std::sqrt(3.0f), 0.25f, 0.25f / std::sqrt(2.0f), 1.0f,                   0.0f,
                           0.0f,  0.0f, std::sqrt(2.0f), -0.25f * std::sqrt(3.0f), 0.25f, 0.25f / std::sqrt(2.0f), 0.5f, std::sqrt(2.0f / 3.0f),
    };

    std::vector<GLuint> pyramidVertexIndices = {
        0,  1,  2,
        3,  4,  5,
        6,  7,  8,
        9, 10, 11,
    };

    std::vector<GLfloat> planeVertexData = {
         1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    };

    std::vector<GLuint> planeVertexIndices = {
        0, 1, 2,
        2, 3, 0,
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

    // Create lamp shader program

    lampVertexShader = createShader(GL_VERTEX_SHADER, lampVertexShaderSource);
    lampFragmentShader = createShader(GL_FRAGMENT_SHADER, lampFragmentShaderSource);
    lampShaderProgram = createProgram({lampVertexShader, lampFragmentShader});
    glDeleteShader(lampVertexShader);
    glDeleteShader(lampFragmentShader);

    // Setup data

    glGenBuffers(vertexBuffers.size(), vertexBuffers.data());
    glGenBuffers(elementBuffers.size(), elementBuffers.data());
    storeData(cubeVertexData, cubeVertexIndices, cubeVBO, cubeEBO);
    storeData(pyramidVertexData, pyramidVertexIndices, pyramidVBO, pyramidEBO);
    storeData(planeVertexData, planeVertexIndices, planeVBO, planeEBO);

    // Setup VAOs

    glGenVertexArrays(vertexArrays.size(), vertexArrays.data());
    setupModel(cubeVAO, cubeVBO, cubeEBO);
    setupModel(pyramidVAO, pyramidVBO, pyramidEBO);
    setupModel(planeVAO, planeVBO, planeEBO);
    setupLamp(pointLightVAO, cubeVBO, cubeEBO);
    setupLamp(spotLightVAO, pyramidVBO, pyramidEBO);
    setupLamp(directionalLightVAO, planeVBO, planeEBO);

    // Setup textures

    glGenTextures(1, &cubeDiffuseMap);
    glGenTextures(1, &cubeSpecularMap);
    loadTextureData(cubeDiffuseMap, cubeDiffuseMapPath);
    loadTextureData(cubeSpecularMap, cubeSpecularMapPath);

    float forwardAxisValue, rightAxisValue, upAxisValue;

    float previousTime = 0.0f;

    while (not glfwWindowShouldClose(window)){
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - previousTime;

        forwardAxisValue = 0.0f;
        rightAxisValue = 0.0f;
        upAxisValue = 0.0f;

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS){
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
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){
            bFlashLight = true;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE){
            bFlashLight = false;
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

        if (bFlashLight and numSpotLights > 0){
            spotLights.back().direction = camera->getCameraForwardVector();
            spotLights.back().position = camera->getCameraPos();
        }

        view = CameraManager::getViewMatrix();
        projection = CameraManager::getProjectionMatrix();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Setup model draw parameters

        glUseProgram(cubeShaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(cubeShaderProgram, "cameraPos"), 1, glm::value_ptr(camera->getCameraPos()));

        glUniform1i(glGetUniformLocation(cubeShaderProgram, "material.diffuse"), 0);
        glUniform1i(glGetUniformLocation(cubeShaderProgram, "material.specular"), 1);
        glUniform1f(glGetUniformLocation(cubeShaderProgram, "material.shininess"), cubeShininess);

        setupPointLights(cubeShaderProgram, pointLights);
        setupSpotLights(cubeShaderProgram, spotLights, bFlashLight);
        setupDirectionalLights(cubeShaderProgram, directionalLights);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeDiffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, cubeSpecularMap);

        // Draw cubes

        for (int i = 0; i < numCubesX; i++){
            for (int j = 0; j < numCubesY; j++){
                bool bPyramid = (i + j) % 3 == 0;
                float offsetX = i * cubeDistanceX - (numCubesX - 1) * cubeDistanceX / 2.0f;
                float offsetY = j * cubeDistanceY - (numCubesY - 1)* cubeDistanceY / 2.0f;
                if (bPyramid){
                    setPyramidUniforms(cubeShaderProgram, offsetX, offsetY);
                    glBindVertexArray(pyramidVAO);
                    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, nullptr);
                }
                else {
                    setCubeUniforms(cubeShaderProgram, offsetX, offsetY);
                    glBindVertexArray(cubeVAO);
                    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
                }
            }
        }

        // Draw floor

        floorModel = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -1.0f}), floorScale);
        floorNormal = glm::transpose(glm::inverse(glm::mat3(floorModel)));
        glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(floorModel));
        glUniformMatrix3fv(glGetUniformLocation(cubeShaderProgram, "normal"), 1, GL_FALSE, glm::value_ptr(floorNormal));
        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Draw lamps

        glUseProgram(lampShaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(lampShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        for (int i = 0; i < numPointLights; i++){
            setPointLightLampUniforms(lampShaderProgram, pointLights[i]);
            glBindVertexArray(pointLightVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        }

        for (int i = 0; i < numSpotLights - 1; i++){
            setSpotLightLampUniforms(lampShaderProgram, spotLights[i]);
            glBindVertexArray(spotLightVAO);
            glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, nullptr);
        }

        for (int i = 0; i < numDirectionalLights; i++){
            setDirectionalLightLampUniforms(lampShaderProgram, directionalLights[i]);
            glBindVertexArray(directionalLightVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        previousTime = currentTime;
    }

    CameraManager::setActiveCamera(nullptr);
    CameraManager::setActiveWindow(nullptr);

    glDeleteTextures(1, &cubeDiffuseMap);
    glDeleteTextures(1, &cubeSpecularMap);
    glDeleteVertexArrays(vertexArrays.size(), vertexArrays.data());
    glDeleteBuffers(vertexBuffers.size(), vertexBuffers.data());
    glDeleteBuffers(elementBuffers.size(), elementBuffers.data());
    glDeleteProgram(cubeShaderProgram);
    glDeleteProgram(lampShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
