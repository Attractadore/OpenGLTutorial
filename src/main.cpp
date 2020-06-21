﻿#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "Lights.hpp"
#include "TextureLoader.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <IL/il.h>
#include <fmt/format.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <pybind11/embed.h>

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <iterator>
#include <iostream>

#include <cmath>

namespace py = pybind11;

std::string loadShaderSource(std::filesystem::path filePath){
    py::object load_shader_source = py::module::import("src.load_shader_source").attr("load_shader_source");
    return load_shader_source(filePath.string()).cast<std::string>();
}

struct Material {
    std::string diffuseMap;
    std::string specularMap;
    float shininess;
};

struct MeshData{
    std::vector<GLfloat> vertexData;
    std::vector<GLuint> vertexIndices;
    Material meshMaterial;
};

void debugFunction(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam​){
    if (severity​ != GL_DEBUG_SEVERITY_NOTIFICATION){
        std::printf("%s\n", message​);
    }
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

void setModelUniforms(GLuint shaderProgram, const glm::mat4& model, const glm::mat3& normal, const Material& material){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId(material.diffuseMap));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId(material.specularMap));
    glUniform1i(glGetUniformLocation(shaderProgram, "material.diffuse"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "material.specular"), 1);
    glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), material.shininess);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normal"), 1, GL_FALSE, glm::value_ptr(normal));
}


void setCubeUniforms(GLuint shaderProgram, float x, float y, const Material& material){
    glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), {x, y, 0.0f});
    glm::mat3 cubeNormal = glm::transpose(glm::inverse(glm::mat3(cubeModel)));
    setModelUniforms(shaderProgram, cubeModel, cubeNormal, material);
}

void setPyramidUniforms(GLuint shaderProgram, float x, float y, const Material& material){
    glm::mat4 model = glm::translate(glm::mat4(1.0f), {x, y, -1.0f});
    model = glm::scale(model, 2.0f * glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(model)));
    setModelUniforms(shaderProgram, model, normal, material);
}

void setFloorUniforms(GLuint shaderProgram, const Material& material){
    glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -1.0f});
    glm::mat3 floorNormal = glm::transpose(glm::inverse(glm::mat3(floorModel)));
    setModelUniforms(shaderProgram, floorModel, floorNormal, material);
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
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(dl.diffuse));
}

std::vector<MeshData> loadModelData(std::filesystem::path modelPath){
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath.c_str(), aiProcess_JoinIdenticalVertices | aiProcess_Triangulate);
    if (!scene){
        return {};
    }
    std::vector<MeshData> meshes;
    for (int i = 0; i < scene->mNumMeshes; i++){\
        MeshData newMeshData;
        auto mesh = scene->mMeshes[i];
        auto meshMaterial = scene->mMaterials[mesh->mMaterialIndex];
        newMeshData.vertexData.reserve(8 * mesh->mNumVertices);
        newMeshData.vertexIndices.reserve(3 * mesh->mNumFaces);
        for (int j = 0; j < mesh->mNumVertices; j++){
            newMeshData.vertexData.push_back(mesh->mVertices[j].x);
            newMeshData.vertexData.push_back(mesh->mVertices[j].y);
            newMeshData.vertexData.push_back(mesh->mVertices[j].z);
            newMeshData.vertexData.push_back(mesh->mNormals[j].x);
            newMeshData.vertexData.push_back(mesh->mNormals[j].y);
            newMeshData.vertexData.push_back(mesh->mNormals[j].z);
            newMeshData.vertexData.push_back(mesh->mTextureCoords[0][j].x);
            newMeshData.vertexData.push_back(mesh->mTextureCoords[0][j].y);
        }
        for (int j = 0; j < mesh->mNumFaces; j++){
            auto face = mesh->mFaces[j];
            for (int k = 0; k < face.mNumIndices; k++){
                newMeshData.vertexIndices.push_back(face.mIndices[k]);
            }
        }
        aiString diffuseTexture;
        aiString specularTexture;
        meshMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexture);
        meshMaterial->GetTexture(aiTextureType_SPECULAR, 0, &specularTexture);
        newMeshData.meshMaterial.diffuseMap = diffuseTexture.C_Str();
        newMeshData.meshMaterial.specularMap = specularTexture.C_Str();
        newMeshData.meshMaterial.shininess = 32.0f;
        meshes.push_back(std::move(newMeshData));
    }
    return meshes;
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

    glm::mat4 view;
    glm::mat4 projection;

    GLuint cubeVertexShader, cubeFragmentShader;
    GLuint lampVertexShader, lampFragmentShader;
    GLuint cubeShaderProgram, lampShaderProgram;

    std::vector<GLuint> vertexBuffers(6);
    std::vector<GLuint> elementBuffers(6);
    std::vector<GLuint> vertexArrays(6);

    GLuint& cubeVBO = vertexBuffers[0];
    GLuint& pyramidVBO = vertexBuffers[1];
    GLuint& circlePlaneVBO = vertexBuffers[2];
    GLuint& sphereVBO = vertexBuffers[3];
    GLuint& coneVBO = vertexBuffers[4];
    GLuint& squarePlaneVBO = vertexBuffers[5];

    GLuint& cubeEBO = elementBuffers[0];
    GLuint& pyramidEBO = elementBuffers[1];
    GLuint& planeEBO = elementBuffers[2];
    GLuint& sphereEBO = elementBuffers[3];
    GLuint& coneEBO = elementBuffers[4];
    GLuint& squarePlaneEBO = elementBuffers[5];

    GLuint& cubeVAO = vertexArrays[0];
    GLuint& pyramidVAO = vertexArrays[1];
    GLuint& floorVAO = vertexArrays[2];
    GLuint& pointLightVAO = vertexArrays[3];
    GLuint& spotLightVAO = vertexArrays[4];
    GLuint& directionalLightVAO = vertexArrays[5];

    auto ts = std::chrono::steady_clock::now();
    py::scoped_interpreter guard{};
    auto cubeVertexShaderSource = loadShaderSource("assets/shaders/triangle.vert");
    auto cubeFragmentShaderSource = loadShaderSource("assets/shaders/triangle.frag");
    auto lampVertexShaderSource = loadShaderSource("assets/shaders/lamp.vert");
    auto lampFragmentShaderSource = loadShaderSource("assets/shaders/lamp.frag");
    auto te = std::chrono::steady_clock::now();
    std::cout << "Load shaders in " << std::chrono::duration_cast<std::chrono::microseconds>(te-ts).count() << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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

    auto [cubeVertexData, cubeVertexIndices, cubeMaterial] = loadModelData("assets/meshes/cube.obj")[0];
    auto [pyramidVertexData, pyramidVertexIndices, pyramidMaterial] = loadModelData("assets/meshes/pyramid.obj")[0];
    auto [circularPlaneVertexData, circularPlaneVertexIndices, circularPlaneMaterial] = loadModelData("assets/meshes/circularplane.obj")[0];
    auto [coneVertexData, coneVertexIndices, coneMaterial] = loadModelData("assets/meshes/cone.obj")[0];
    auto [sphereVertexData, sphereVertexIndices, sphereMaterial] = loadModelData("assets/meshes/sphere.obj")[0];
    auto [squarePlaneVertexData, squarePlaneVertexIndices, squarePlaneMaterial] = loadModelData("assets/meshes/squareplane.obj")[0];

    // Setup data

    glGenBuffers(vertexBuffers.size(), vertexBuffers.data());
    glGenBuffers(elementBuffers.size(), elementBuffers.data());
    storeData(cubeVertexData, cubeVertexIndices, cubeVBO, cubeEBO);
    storeData(pyramidVertexData, pyramidVertexIndices, pyramidVBO, pyramidEBO);
    storeData(circularPlaneVertexData, circularPlaneVertexIndices, circlePlaneVBO, planeEBO);
    storeData(sphereVertexData, sphereVertexIndices, sphereVBO, sphereEBO);
    storeData(coneVertexData, coneVertexIndices, coneVBO, coneEBO);
    storeData(squarePlaneVertexData, squarePlaneVertexIndices, squarePlaneVBO, squarePlaneEBO);

    // Setup VAOs

    glGenVertexArrays(vertexArrays.size(), vertexArrays.data());
    setupModel(cubeVAO, cubeVBO, cubeEBO);
    setupModel(pyramidVAO, pyramidVBO, pyramidEBO);
    setupModel(floorVAO, circlePlaneVBO, planeEBO);
    setupLamp(pointLightVAO, sphereVBO, sphereEBO);
    setupLamp(spotLightVAO, coneVBO, coneEBO);
    setupLamp(directionalLightVAO, squarePlaneVBO, squarePlaneEBO);

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

        setupPointLights(cubeShaderProgram, pointLights);
        setupSpotLights(cubeShaderProgram, spotLights, bFlashLight);
        setupDirectionalLights(cubeShaderProgram, directionalLights);

        // Draw cubes

        for (int i = 0; i < numCubesX; i++){
            for (int j = 0; j < numCubesY; j++){
                bool bPyramid = (i + j) % 3 == 0;
                float offsetX = i * cubeDistanceX - (numCubesX - 1) * cubeDistanceX / 2.0f;
                float offsetY = j * cubeDistanceY - (numCubesY - 1) * cubeDistanceY / 2.0f;
                if (bPyramid){
                    setPyramidUniforms(cubeShaderProgram, offsetX, offsetY, pyramidMaterial);
                    glBindVertexArray(pyramidVAO);
                    glDrawElements(GL_TRIANGLES, pyramidVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
                }
                else {
                    setCubeUniforms(cubeShaderProgram, offsetX, offsetY, cubeMaterial);
                    glBindVertexArray(cubeVAO);
                    glDrawElements(GL_TRIANGLES, cubeVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
                }
            }
        }

        // Draw floor
        setFloorUniforms(cubeShaderProgram, circularPlaneMaterial);
        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, circularPlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);

        // Draw lamps

        glUseProgram(lampShaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(lampShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        for (int i = 0; i < numPointLights; i++){
            setPointLightLampUniforms(lampShaderProgram, pointLights[i]);
            glBindVertexArray(pointLightVAO);
            glDrawElements(GL_TRIANGLES, sphereVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        for (int i = 0; i < numSpotLights - 1; i++){
            setSpotLightLampUniforms(lampShaderProgram, spotLights[i]);
            glBindVertexArray(spotLightVAO);
            glDrawElements(GL_TRIANGLES, coneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        for (int i = 0; i < numDirectionalLights; i++){
            setDirectionalLightLampUniforms(lampShaderProgram, directionalLights[i]);
            glBindVertexArray(directionalLightVAO);
            glDrawElements(GL_TRIANGLES, squarePlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        previousTime = currentTime;
    }

    CameraManager::setActiveCamera(nullptr);
    CameraManager::setActiveWindow(nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
