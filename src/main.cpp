#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "Lights.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <GLFW/glfw3.h>
#include <IL/il.h>

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

std::vector<char> loadShaderBinary(std::filesystem::path filePath){
    std::ifstream is(filePath, std::ios::binary);
    if (!is){
        return {};
    }
    auto fileSize = std::filesystem::file_size(filePath);
    std::vector<char> fileContents(fileSize);
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

void loadTextureData(GLuint textureId, std::filesystem::path filePath){
    if (!std::filesystem::exists(filePath)){
        return;
    }
    ILuint loadedImage;
    ilGenImages(1, &loadedImage);
    ilBindImage(loadedImage);
    ilLoadImage(filePath.c_str());
    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    int levels = std::max(std::log2(width), std::log2(height));
    std::vector<std::byte> imageData(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, imageData.data());
    glTextureStorage2D(textureId, levels, GL_RGBA8, width, height);
    glTextureSubImage2D(textureId, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
    glGenerateTextureMipmap(textureId);
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

GLuint createShader(GLenum shaderType, std::vector<char> shaderBinary, std::string entryPoint = "main"){
    GLuint shader = glCreateShader(shaderType);
    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderBinary.data(), shaderBinary.size());
    glSpecializeShader(shader, entryPoint.c_str(), 0, nullptr, nullptr);
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

void setupModel(GLuint VAO, GLuint VBO, GLuint EBO){
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 8 * sizeof(GLfloat));
    glVertexArrayElementBuffer(VAO, EBO);
    glEnableVertexArrayAttrib(VAO, 0);
    glEnableVertexArrayAttrib(VAO, 1);
    glEnableVertexArrayAttrib(VAO, 2);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GLfloat));
    glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof (GLfloat));
    glVertexArrayAttribBinding(VAO, 0, 0);
    glVertexArrayAttribBinding(VAO, 1, 0);
    glVertexArrayAttribBinding(VAO, 2, 0);
}

void setupLamp(GLuint VAO, GLuint VBO, GLuint EBO){
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 8 * sizeof (GLfloat));
    glVertexArrayElementBuffer(VAO, EBO);
    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 0, 0);
}

void setCubeUniforms(GLuint shaderProgram, float x, float y){
    glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), {x, y, 0.0f});
    glm::mat3 cubeNormal = glm::transpose(glm::inverse(glm::mat3(cubeModel)));
    glProgramUniformMatrix4fv(shaderProgram, 0, 1, GL_FALSE, glm::value_ptr(cubeModel));
    glProgramUniformMatrix3fv(shaderProgram, 3, 1, GL_FALSE, glm::value_ptr(cubeNormal));
}

void setPyramidUniforms(GLuint shaderProgram, float x, float y){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), {x, y, -1.0f});
    model = glm::scale(model, 2.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
    glProgramUniformMatrix4fv(shaderProgram, 0, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniformMatrix3fv(shaderProgram, 3, 1, GL_FALSE, glm::value_ptr(normal));
}

glm::vec3 normalizeColor(glm::vec3 color){
    color = glm::normalize(color);
    color /= std::max(color.r, std::max(color.g, color.b));
    return color;
}

void setPointLightLampUniforms(GLuint shaderProgram, const PointLight& pl){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pl.position);
    glm::mat4 local = glm::scale(glm::mat4(1.0f), pl.minRadius * glm::vec3(1.0f, 1.0f, 1.0f));
    model = model * local;
    glProgramUniformMatrix4fv(shaderProgram, 0, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniformMatrix4fv(shaderProgram, 3, 1, GL_FALSE, glm::value_ptr(local));
    glProgramUniform3fv(shaderProgram, 4, 1, glm::value_ptr(pl.diffuse));
    glProgramUniform1f(shaderProgram, 5, pl.minRadius);
    glProgramUniform1f(shaderProgram, 6, pl.maxRadius);
    glProgramUniform1i(shaderProgram, 7, false);
}

void setSpotLightLampUniforms(GLuint shaderProgram, const SpotLight& sl){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), sl.position);
    float angle = glm::acos(glm::dot(glm::normalize(sl.direction), {0.0f, 0.0f, -1.0f}));
    glm::vec3 rotateAxis = glm::normalize(glm::cross({0.0f, 0.0f, -1.0f}, sl.direction));
    model = glm::rotate(model, angle, rotateAxis);
    float scaleXY = sl.minRadius * std::sin(std::acos(sl.outerAngleCos));
    float scaleZ = sl.minRadius * sl.outerAngleCos / std::sqrt(2.0f);
    glm::mat local = glm::scale(glm::mat4(1.0f), {scaleXY, scaleXY, scaleZ});
    local = glm::translate(local, {0.0f, 0.0f, -std::sqrt(2.0f)});
    model = model * local;
    glProgramUniformMatrix4fv(shaderProgram, 0, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniformMatrix4fv(shaderProgram, 3, 1, GL_FALSE, glm::value_ptr(local));
    glProgramUniform3fv(shaderProgram, 4, 1, glm::value_ptr(sl.diffuse));
    glProgramUniform1f(shaderProgram, 5, sl.minRadius);
    glProgramUniform1f(shaderProgram, 6, sl.maxRadius);
    glProgramUniform1i(shaderProgram, 7, true);
    glProgramUniform1f(shaderProgram, 8, sl.innerAngleCos);
    glProgramUniform1f(shaderProgram, 9, sl.outerAngleCos);
}

void setDirectionalLightLampUniforms(GLuint shaderProgram, const DirectionalLight& dl){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 30.0f});
    float angle = glm::acos(glm::dot(glm::normalize(dl.direction), {0.0f, 0.0f, -1.0f}));
    glm::vec3 rotateAxis = glm::normalize(glm::cross({0.0f, 0.0f, -1.0f}, dl.direction));
    model = glm::rotate(model, angle, rotateAxis);
    model = glm::scale(model, 10.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    glProgramUniformMatrix4fv(shaderProgram, 0, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniform3fv(shaderProgram, 3, 1, glm::value_ptr(dl.diffuse));
}

struct Material {
    GLuint diffuseMap;
    GLuint specularMap;
    float shininess;
};

int main(){
    std::string windowTitle = "OpenGL Tutorial";
    float horizontalFOV = 90.0f;
    float aspectRatio = 16.0f / 9.0f;
    int defaultWindowHeight = 720;
    int defaultWindowWidth = defaultWindowHeight * aspectRatio;
    float cameraSpeed = 5.0f;

    const int MAX_POINT_LIGHTS = 10;
    const int MAX_SPOT_LIGHTS = 10;
    const int MAX_DIRECTIONAL_LIGHTS = 1;

    int numPointLights = 10;
    int numSpotLights = 5 + 1;
    int numDirectionalLights = 1;
    numDirectionalLights = std::min(numDirectionalLights, 1);
    bool bFlashLight = false;

    numPointLights = std::min(numPointLights, MAX_POINT_LIGHTS);
    numSpotLights = std::min(numSpotLights, MAX_SPOT_LIGHTS);
    numDirectionalLights = std::min(numDirectionalLights, MAX_DIRECTIONAL_LIGHTS);

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
    Material cubeMaterial;

    GLuint cubeVertexShader, cubeFragmentShader;
    GLuint lampVertexShader, lampFragmentShader;
    GLuint litLampVertexShader, litLampFragmentShader;
    GLuint cubeShaderProgram, lampShaderProgram, litLampShaderProgram;
    GLuint cubeDiffuseMap, cubeSpecularMap;
    GLuint textureSampler;

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

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
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
    glEnable(GL_MULTISAMPLE);

    // Load shader binaries

    auto cubeVertexShaderBinary = loadShaderBinary("assets/shaders/bin/triangle.vert");
    auto cubeFragmentShaderBinary = loadShaderBinary("assets/shaders/bin/triangle.frag");
    auto lampVertexShaderBinary = loadShaderBinary("assets/shaders/bin/lamp.vert");
    auto lampFragmentShaderBinary = loadShaderBinary("assets/shaders/bin/lamp.frag");
    auto litLampVertexShaderBinary = loadShaderBinary("assets/shaders/bin/litLamp.vert");
    auto litLampFragmentShaderBinary = loadShaderBinary("assets/shaders/bin/litLamp.frag");

    // Create cube shader program

    cubeVertexShader = createShader(GL_VERTEX_SHADER, cubeVertexShaderBinary);
    cubeFragmentShader = createShader(GL_FRAGMENT_SHADER, cubeFragmentShaderBinary);
    cubeShaderProgram = createProgram({cubeVertexShader, cubeFragmentShader});
    glDeleteShader(cubeVertexShader);
    glDeleteShader(cubeFragmentShader);

    // Create lamp shader programs

    lampVertexShader = createShader(GL_VERTEX_SHADER, lampVertexShaderBinary);
    lampFragmentShader = createShader(GL_FRAGMENT_SHADER, lampFragmentShaderBinary);
    lampShaderProgram = createProgram({lampVertexShader, lampFragmentShader});
    glDeleteShader(lampVertexShader);
    glDeleteShader(lampFragmentShader);

    litLampVertexShader = createShader(GL_VERTEX_SHADER, litLampVertexShaderBinary);
    litLampFragmentShader = createShader(GL_FRAGMENT_SHADER, litLampFragmentShaderBinary);
    litLampShaderProgram = createProgram({litLampVertexShader, litLampFragmentShader});
    glDeleteShader(litLampVertexShader);
    glDeleteShader(litLampFragmentShader);

    // Setup data

    glCreateBuffers(vertexBuffers.size(), vertexBuffers.data());
    glCreateBuffers(elementBuffers.size(), elementBuffers.data());
    glNamedBufferData(cubeVBO, sizeof (GLfloat) * cubeVertexData.size(), cubeVertexData.data(), GL_STATIC_DRAW);
    glNamedBufferData(cubeEBO, sizeof (GLuint) * cubeVertexIndices.size(), cubeVertexIndices.data(), GL_STATIC_DRAW);
    glNamedBufferData(pyramidVBO, sizeof (GLfloat) * pyramidVertexData.size(), pyramidVertexData.data(), GL_STATIC_DRAW);
    glNamedBufferData(pyramidEBO, sizeof (GLuint) * pyramidVertexIndices.size(), pyramidVertexIndices.data(), GL_STATIC_DRAW);
    glNamedBufferData(planeVBO, sizeof (GLfloat) * planeVertexData.size(), planeVertexData.data(), GL_STATIC_DRAW);
    glNamedBufferData(planeEBO, sizeof (GLuint) * planeVertexIndices.size(), planeVertexIndices.data(), GL_STATIC_DRAW);

    // Setup VAOs

    glCreateVertexArrays(vertexArrays.size(), vertexArrays.data());
    setupModel(cubeVAO, cubeVBO, cubeEBO);
    setupModel(pyramidVAO, pyramidVBO, pyramidEBO);
    setupModel(planeVAO, planeVBO, planeEBO);
    setupLamp(pointLightVAO, cubeVBO, cubeEBO);
    setupLamp(spotLightVAO, pyramidVBO, pyramidEBO);
    setupLamp(directionalLightVAO, planeVBO, planeEBO);

    // Setup textures

    glCreateTextures(GL_TEXTURE_2D, 1, &cubeDiffuseMap);
    glCreateTextures(GL_TEXTURE_2D, 1, &cubeSpecularMap);
    loadTextureData(cubeDiffuseMap, cubeDiffuseMapPath);
    loadTextureData(cubeSpecularMap, cubeSpecularMapPath);

    // Setup texture sampling

    glCreateSamplers(1, &textureSampler);
    glSamplerParameteri(textureSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(textureSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(textureSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(textureSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    float maxAF;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAF);
    glSamplerParameterf(textureSampler, GL_TEXTURE_MAX_ANISOTROPY, maxAF);

    float forwardAxisValue, rightAxisValue, upAxisValue;

    float previousTime = 0.0f;

    cubeMaterial.diffuseMap = cubeDiffuseMap;
    cubeMaterial.specularMap = cubeSpecularMap;
    cubeMaterial.shininess = cubeShininess;

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

        // Set common model uniforms

        glProgramUniformMatrix4fv(cubeShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));
        glProgramUniformMatrix4fv(cubeShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(projection));

        glProgramUniform1i(cubeShaderProgram, 4, 0);
        glProgramUniform1i(cubeShaderProgram, 5, 1);
        glProgramUniform1f(cubeShaderProgram, 6, cubeMaterial.shininess);
        glProgramUniform3fv(cubeShaderProgram, 7, 1, glm::value_ptr(camera->getCameraPos()));
        glProgramUniform1i(cubeShaderProgram, 8, numDirectionalLights);
        for (int i = 0; i < numDirectionalLights; i++){
            glProgramUniform3fv(cubeShaderProgram, 11, 1, glm::value_ptr(directionalLights[i].direction));
            glProgramUniform1f(cubeShaderProgram, 12, directionalLights[i].ambient);
            glProgramUniform3fv(cubeShaderProgram, 13, 1, glm::value_ptr(directionalLights[i].diffuse));
            glProgramUniform1f(cubeShaderProgram, 14, directionalLights[i].specular);
        }
        glProgramUniform1i(cubeShaderProgram, 9, numPointLights);
        for (int i = 0; i < numPointLights; i++){
            int location = 15 + i * 6;
            glProgramUniform3fv(cubeShaderProgram, location, 1, glm::value_ptr(pointLights[i].position));
            glProgramUniform1f(cubeShaderProgram, location + 1, pointLights[i].minRadius);
            glProgramUniform1f(cubeShaderProgram, location + 2, pointLights[i].maxRadius);
            glProgramUniform1f(cubeShaderProgram, location + 3, pointLights[i].ambient);
            glProgramUniform3fv(cubeShaderProgram, location + 4, 1, glm::value_ptr(pointLights[i].diffuse));
            glProgramUniform1f(cubeShaderProgram, location + 5, pointLights[i].specular);
        }
        glProgramUniform1i(cubeShaderProgram, 10, numSpotLights - !bFlashLight);
        for (int i = 0; i < numSpotLights - !bFlashLight; i++){
            int location = 15 + 10 * 6 + i * 9;
            glProgramUniform3fv(cubeShaderProgram, location, 1, glm::value_ptr(spotLights[i].position));
            glProgramUniform3fv(cubeShaderProgram, location + 1, 1, glm::value_ptr(spotLights[i].direction));
            glProgramUniform1f(cubeShaderProgram, location + 2, spotLights[i].innerAngleCos);
            glProgramUniform1f(cubeShaderProgram, location + 3, spotLights[i].outerAngleCos);
            glProgramUniform1f(cubeShaderProgram, location + 4, spotLights[i].minRadius);
            glProgramUniform1f(cubeShaderProgram, location + 5, spotLights[i].maxRadius);
            glProgramUniform1f(cubeShaderProgram, location + 6, spotLights[i].ambient);
            glProgramUniform3fv(cubeShaderProgram, location + 7, 1, glm::value_ptr(spotLights[i].diffuse));
            glProgramUniform1f(cubeShaderProgram, location + 8, spotLights[i].specular);
        }

        // Setup textures

        glBindTextureUnit(0, cubeMaterial.diffuseMap);
        glBindSampler(0, textureSampler);
        glBindTextureUnit(1, cubeMaterial.specularMap);
        glBindSampler(1, textureSampler);

        // Draw cubes

        glUseProgram(cubeShaderProgram);

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
        glProgramUniformMatrix4fv(cubeShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(floorModel));
        glProgramUniformMatrix3fv(cubeShaderProgram, 3, 1, GL_FALSE, glm::value_ptr(floorNormal));
        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Set common lit lamp uniforms

        glProgramUniformMatrix4fv(litLampShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));
        glProgramUniformMatrix4fv(litLampShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw lit lamps

        glUseProgram(litLampShaderProgram);

        for (int i = 0; i < numPointLights; i++){
            setPointLightLampUniforms(litLampShaderProgram, pointLights[i]);
            glBindVertexArray(pointLightVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        }

        for (int i = 0; i < numSpotLights - 1; i++){
            setSpotLightLampUniforms(litLampShaderProgram, spotLights[i]);
            glBindVertexArray(spotLightVAO);
            glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, nullptr);
        }

        // Set lamp uniforms

        glProgramUniformMatrix4fv(lampShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));
        glProgramUniformMatrix4fv(lampShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw lamps

        glUseProgram(lampShaderProgram);

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
    glDeleteSamplers(1, &textureSampler);
    glDeleteVertexArrays(vertexArrays.size(), vertexArrays.data());
    glDeleteBuffers(vertexBuffers.size(), vertexBuffers.data());
    glDeleteBuffers(elementBuffers.size(), elementBuffers.data());
    glDeleteProgram(cubeShaderProgram);
    glDeleteProgram(lampShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
