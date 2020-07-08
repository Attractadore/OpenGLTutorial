﻿#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "Lights.hpp"
#include "TextureLoader.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <fstream>

struct Material {
    std::string diffuseMap;
    std::string specularMap;
    float shininess;
};

bool operator==(const Material& m1, const Material& m2){
    return m1.diffuseMap == m2.diffuseMap and m1.specularMap == m2.specularMap and m1.shininess == m2.shininess;
}

bool operator!=(const Material& m1, const Material& m2){
    return m1.diffuseMap != m2.diffuseMap or m1.specularMap != m2.specularMap or m1.shininess != m2.shininess;
}

struct MeshData{
    std::vector<GLfloat> vertexData;
    std::vector<GLuint> vertexIndices;
    Material meshMaterial;
};

void debugFunction(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*){
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION){
        std::printf("%s\n", message);
    }
}

std::string loadShaderSource(std::filesystem::path filePath){
    std::ifstream is(filePath);
    if (!is){
        return "";
    }
    auto fileSize = std::filesystem::file_size(filePath);
    std::string fileContents(fileSize, ' ');
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

GLuint createShader(GLenum shaderType, std::string shaderSource){
    const char * shaderSourceCStr = shaderSource.c_str();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSourceCStr, nullptr);
    glCompileShader(shader);
    return shader;
}

GLuint createProgram(const std::vector<GLuint>& shaders){
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

template <typename S1, typename S2>
void storeData(const S1& vertexData, const S2& vertexIndices, GLuint VBO, GLuint EBO) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof (typename S1::value_type) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (typename S2::value_type) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);
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

void setupRenderRect(GLuint VAO, GLuint VBO, GLuint EBO) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *) (2 * sizeof (GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}

void setShaderMatrial(GLuint shaderProgram, const Material& material){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(material.diffuseMap));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(material.specularMap));
    glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), material.shininess);
}

void setModelUniforms(GLuint shaderProgram, const glm::mat4& model, const glm::mat3& normal){
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normal"), 1, GL_FALSE, glm::value_ptr(normal));
}

void setLampUniforms(GLuint shaderProgram, const glm::mat4& model, const glm::vec3& lightColor){
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
}

std::vector<MeshData> loadModelData(const std::string& modelPath){
    if (!std::filesystem::exists(modelPath)){
        return {};
    }
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

template <typename P>
void copyLightColor(P* dst, const LightCommon& light){
    std::memcpy(dst + 00, glm::value_ptr(light.ambient), 12);
    std::memcpy(dst + 16, glm::value_ptr(light.diffuse), 12);
    std::memcpy(dst + 32, glm::value_ptr(light.specular), 12);
}

template <typename P>
void copyLightAttenuation(P* dst, const PointLight& light){
    std::memcpy(dst + 0, &light.kc, 4);
    std::memcpy(dst + 4, &light.kl, 4);
    std::memcpy(dst + 8, &light.kq, 4);
}

template <typename P>
void copyLightPosition(P* dst, const PointLight& light){
    std::memcpy(dst, glm::value_ptr(light.position), 12);
}

template <typename P>
void copyLightDirection(P* dst, const DirectionalLight& light){
    std::memcpy(dst, glm::value_ptr(light.direction), 12);
}

template <typename P>
void copyLightAngle(P* dst, const SpotLight& light){
    std::memcpy(dst + 0, &light.innerAngleCos, 4);
    std::memcpy(dst + 4, &light.outerAngleCos, 4);
}

template <typename S>
void calculatePyramidMatrices(int w, int h, float dX, float dY, S& matrices){
    for (int i = 0; i < w; i++){
        for (int j = 0; j < h; j++){
            if ((i + j) % 3){
                continue;
            }
            float x = i * dX - (w - 1) * dX / 2.0f;
            float y = j * dY - (h - 1) * dY / 2.0f;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), {x, y, -1.0f});
            model = glm::scale(model, 2.0f * glm::vec3(1.0f, 1.0f, 1.0f));
            glm::mat3 normal(1.0f);
            matrices.emplace_back(model, normal);
        }
    }
}

template <typename S>
void calculateCubeMatrices(int w, int h, float dX, float dY, S& matrices){
    for (int i = 0; i < w; i++){
        for (int j = 0; j < h; j++){
            if (!((i + j) % 3)){
                continue;
            }
            float x = i * dX - (w - 1) * dX / 2.0f;
            float y = j * dY - (h - 1) * dY / 2.0f;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), {x, y, 0.0f});
            glm::mat3 normal(1.0f);
            matrices.emplace_back(model, normal);
        }
    }
}

template <typename S1, typename S2>
void calculatePointLightMatrices(const S1& lights, S2& matrices){
    for (const auto& light : lights){
        glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
        model = glm::scale(model, 0.2f * glm::vec3(1.0f, 1.0f, 1.0f));
        matrices.push_back(model);
    }
}

template <typename S1, typename S2>
void calculateSpotLightMatrices(const S1& lights, S2& matrices){
    for (const auto& light : lights){
        glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
        float angle = glm::acos(glm::dot(glm::normalize(light.direction), {0.0f, 0.0f, -1.0f}));
        glm::vec3 rotateAxis = glm::normalize(glm::cross({0.0f, 0.0f, -1.0f}, light.direction));
        model = glm::rotate(model, angle, rotateAxis);
        model = glm::scale(model, 0.4f * glm::vec3(1.0f, 1.0f, 1.0f));
        matrices.push_back(model);
    }
}

template <typename S1, typename S2>
void calculateDirectionalLightMatrices(const S1& lights, S2& matrices){
    for (const auto& light : lights){
        glm::mat4 model = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 30.0f});
        float angle = glm::acos(glm::dot(glm::normalize(light.direction), {0.0f, 0.0f, -1.0f}));
        glm::vec3 rotateAxis = glm::normalize(glm::cross({0.0f, 0.0f, -1.0f}, light.direction));
        model = glm::rotate(model, angle, rotateAxis);
        matrices.push_back(model);
    }
}

template <typename S>
void setupWindows(S& windows, float distX, float distY, const Material& windowMaterial){
    constexpr std::array<glm::vec3, 4> points = {
        glm::vec3{-1.0f,  1.0f, 0.0f},
        glm::vec3{ 1.0f,  1.0f, 0.0f},
        glm::vec3{ 1.0f, -1.0f, 0.0f},
        glm::vec3{-1.0f, -1.0f, 0.0f},
    };
    for (int i = 0; i < 4; i++){
        glm::vec3 dir = glm::normalize(points[(i + 1) % 4] - points[i]);
        float dist;
        if (i % 1){
            dist = distY;
        }
        else{
            dist = distX;
        }
        glm::vec3 start = points[i] * dist;
        start += dir * (1.0f + std::remainder(2.0f * dist, 2.0f));
        glm::mat4 model(1.0f);
        if (i % 2){
            model = glm::rotate(model, glm::radians(90.0f), {0.0f, 0.0f, 1.0f});
        }
        glm::mat3 normal = glm::mat3(model);
        for (int j = 0; j < std::floor(dist); j++){
            glm::mat4 windowModel = glm::translate(glm::mat4(1.0f), start) * model;
            windows.emplace_back(windowModel, normal, windowMaterial);
            start += 2.0f * dir;
        }
    }

}

template <typename S>
void setupGrass(S& grass, float distX, float distY, const Material& grassMaterial){
    float radius = std::sqrt(distX * distX + distY * distY) + 1.0f;
    int numGrassTufts = std::floor(glm::radians(360.0f) * radius);
    float angleStep = glm::radians(360.0f) / numGrassTufts;
    glm::vec3 dir = {0.0f, 1.0f, 0.0f};
    for (int i = 0; i < numGrassTufts; i++){
        float angle = angleStep * i;
        glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, {0.0f, 0.0f, 1.0f});
        glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::mat3(rotMat) * dir * radius);
        glm::mat4 model = transMat * rotMat;
        glm::mat3 normal = model;
        grass.emplace_back(model, normal, grassMaterial);
    }
}

void swapBuffers(int& readBufferIndex){
    readBufferIndex = !readBufferIndex;
    glReadBuffer(GL_COLOR_ATTACHMENT0 + readBufferIndex);
    glDrawBuffer(GL_COLOR_ATTACHMENT0 + !readBufferIndex);
}

int main(){
    float horizontalFOV = 90.0f;
    int windowW, windowH;
    float windowAspectRatio = 16.0f / 9.0f;
    windowH = 720.0f;
    windowW = windowH * windowAspectRatio;
    float cameraSpeed = 5.0f;

    constexpr int MAX_POINT_LIGHTS = 10,
                  MAX_SPOT_LIGHTS = 10,
                  MAX_DIRECTIONAL_LIGHTS = 10;

    int numPointLights = 5,
        numSpotLights = 5 + 1,
        numDirectionalLights = 1;

    numPointLights = std::min(numPointLights, MAX_POINT_LIGHTS);
    numSpotLights = std::min(numSpotLights, MAX_SPOT_LIGHTS);
    numDirectionalLights = std::min(numDirectionalLights, MAX_DIRECTIONAL_LIGHTS);

    std::vector<PointLight> pointLights(numPointLights);
    std::vector<SpotLight> spotLights(numSpotLights);
    std::vector<DirectionalLight> directionalLights(numDirectionalLights);

    bool bFlashLight = false,
         bGreyScale = false,
         bBloom = true,
         bTAA = false,
         bMSAA = true,
         bSkybox = true,
         bShowMag = false;
    float bloomIntencity = 5.0f;
    int TAASamples = 4;
    std::vector<glm::vec3> TAASamplesPositions = {
        {-0.25f, -0.25f, 0.0f},
        { 0.25f, -0.25f, 0.0f},
        { 0.25f,  0.25f, 0.0f},
        {-0.25f,  0.25f, 0.0f},
    };
    int MSAASamples = 4;

    int numCubesX = 10,
        numCubesY = numCubesX;
    float cubeDistanceX = 4.0f,
          cubeDistanceY = cubeDistanceX;

    std::vector<std::pair<glm::mat4, glm::mat3>> pyramidMatrices,
                                                 cubeMatrices;
    std::vector<glm::mat4> pointLightMatrices,
                           spotLightMatrices,
                           directionalLightMatrices;

    glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -1.01f}),
              floorNormal(1.0f);

    calculatePyramidMatrices(numCubesX, numCubesY, cubeDistanceX, cubeDistanceY, pyramidMatrices);
    calculateCubeMatrices(numCubesX, numCubesY, cubeDistanceX, cubeDistanceY, cubeMatrices);
    calculatePointLightMatrices(pointLights, pointLightMatrices);
    calculateSpotLightMatrices(spotLights, spotLightMatrices);
    calculateDirectionalLightMatrices(directionalLights, directionalLightMatrices);

    Material windowMaterial = { "window.png", "window_specular.png", 64.0f },
             grassMaterial = { "grass.png", "grass_specular.png", 16.0f };
    std::vector<std::string> cubeMapFaceTextures = {
        "skybox/front.png",
        "skybox/back.png",
        "skybox/right.png",
        "skybox/left.png",
        "skybox/top.png",
        "skybox/bottom.png"
    };

    std::vector<std::tuple<glm::mat4, glm::mat3, Material>> transparentObjects;
    float windowRectDW = numCubesX * cubeDistanceX / 2.0f + 2.0f,
          windowRectDH = numCubesY * cubeDistanceY / 2.0f + 2.0f;
    setupWindows(transparentObjects, windowRectDW, windowRectDH, windowMaterial);
    setupGrass(transparentObjects, windowRectDW, windowRectDH, grassMaterial);

    glm::vec3 cameraStartPos = {-5.0f, 0.0f, 3.0f},
              cameraStartLookDirection = -cameraStartPos;

    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDirection, glm::vec3(0.0f, 0.0f, 1.0f));

    GLuint cubeShaderProgram,
            lampShaderProgram,
            TAAShaderProgram,
            greyscaleShaderProgram,
            bloomExtractShaderProgram,
            bloomCombineShaderProgram,
            screenRectShaderProgram,
            blurShaderProgram,
            cubeMapShaderProgram;
    GLuint frameTextureArray;
    std::vector<GLuint> fullResPPTextures(2),
                        quarterResPPTextures(2);
    int colorIndex = 0,
        fullResReadIndex = 0,
        quarterResReadIndex = 0;

    std::vector<GLuint> vertexBuffers(10);

    GLuint& cubeVBO = vertexBuffers[0];
    GLuint& pyramidVBO = vertexBuffers[1];
    GLuint& circlePlaneVBO = vertexBuffers[2];
    GLuint& sphereVBO = vertexBuffers[3];
    GLuint& coneVBO = vertexBuffers[4];
    GLuint& squarePlaneVBO = vertexBuffers[5];
    GLuint& screenRectVBO = vertexBuffers[6];
    GLuint& magRectVBO = vertexBuffers[7];
    GLuint& transparentVBO = vertexBuffers[8];
    GLuint& skyboxVBO = vertexBuffers[9];

    std::vector<GLuint> elementBuffers(9);

    GLuint& cubeEBO = elementBuffers[0];
    GLuint& pyramidEBO = elementBuffers[1];
    GLuint& planeEBO = elementBuffers[2];
    GLuint& sphereEBO = elementBuffers[3];
    GLuint& coneEBO = elementBuffers[4];
    GLuint& squarePlaneEBO = elementBuffers[5];
    GLuint& screenRectEBO = elementBuffers[6];
    GLuint& transparentEBO = elementBuffers[7];
    GLuint& skyboxEBO = elementBuffers[8];

    std::vector<GLuint> uniformBuffers(2);
    GLuint& matrixUBO = uniformBuffers[0];
    GLuint& lightUBO = uniformBuffers[1];

    std::vector<GLuint> frameBuffers(4);
    GLuint& PPFBO = frameBuffers[0];
    GLuint& MSFBO = frameBuffers[1];
    GLuint& blitFBO = frameBuffers[2];
    GLuint& QRFBO = frameBuffers[3];

    std::vector<GLuint> renderBuffers(3);
    GLuint& MSColorRenderBuffer = renderBuffers[0];
    GLuint& MSDepthStencilRenderBuffer = renderBuffers[1];
    GLuint& blitDepthStencilRenderBuffer = renderBuffers[2];

    std::vector<GLuint> vertexArrays(10);

    GLuint& cubeVAO = vertexArrays[0];
    GLuint& pyramidVAO = vertexArrays[1];
    GLuint& floorVAO = vertexArrays[2];
    GLuint& pointLightVAO = vertexArrays[3];
    GLuint& spotLightVAO = vertexArrays[4];
    GLuint& directionalLightVAO = vertexArrays[5];
    GLuint& screenRectVAO = vertexArrays[6];
    GLuint& magRectVAO = vertexArrays[7];
    GLuint& transparentVAO = vertexArrays[8];
    GLuint& skyboxVAO = vertexArrays[9];

    constexpr std::array<GLfloat, 16> screenRectVertexData = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    constexpr std::array<GLfloat, 16> magRectVertexData = {
        -1.0f,  0.0f,   0.0f,   0.0f,
         0.0f,  0.0f, 0.125f,   0.0f,
         0.0f,  1.0f, 0.125f, 0.125f,
        -1.0f,  1.0f,   0.0f, 0.125f
    };

    constexpr std::array<GLuint, 6> rectVertexIndices = {
        0, 1, 2,
        2, 3, 0
    };

    constexpr std::array<GLfloat, 64> transparantObjectVertexData = {
        1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f,-1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
       -1.0f, 0.0f,-1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
       -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f,-1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f,
       -1.0f, 0.0f,-1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,
       -1.0f, 0.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    };

    constexpr std::array<GLuint, 12> transparentObjectVertexIndices = {
        0, 3, 2,
        2, 1, 0,
        4, 5, 6,
        6, 7, 4
    };

    constexpr std::array<GLfloat, 24> skyboxVertexData = {
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f
    };

    constexpr std::array<GLuint, 36> skyboxVertexIndices = {
        1, 2, 0,
        2, 3, 0,
        6, 5, 4,
        7, 6, 4,
        6, 2, 1,
        5, 6, 1,
        3, 7, 0,
        7, 4, 0,
        4, 5, 1,
        4, 1, 0,
        7, 3, 2,
        6, 7, 2
    };

    CameraManager::initialize(windowW, windowH);
    CameraManager::setHorizontalFOV(horizontalFOV);
    CameraManager::currentCamera = camera;
    CameraManager::enableCameraLook();
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    if (glIsEnabled(GL_DEBUG_OUTPUT)){
        glDebugMessageCallback(debugFunction, nullptr);
    }
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup rendering textures

    glGenTextures(fullResPPTextures.size(), fullResPPTextures.data());
    for (const auto& tex : fullResPPTextures){
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowW, windowH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glGenTextures(quarterResPPTextures.size(), quarterResPPTextures.data());
    for (const auto& tex : quarterResPPTextures){
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowW / 2, windowH / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glGenTextures(1, &frameTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, frameTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, windowW, windowH, TAASamples, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Setup renderbuffers

    glGenRenderbuffers(renderBuffers.size(), renderBuffers.data());
    glBindRenderbuffer(GL_RENDERBUFFER, MSColorRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAASamples, GL_RGBA, windowW, windowH);
    glBindRenderbuffer(GL_RENDERBUFFER, MSDepthStencilRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAASamples, GL_DEPTH24_STENCIL8, windowW, windowH);
    glBindRenderbuffer(GL_RENDERBUFFER, blitDepthStencilRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH24_STENCIL8, windowW, windowH);

    // Setup framebuffers

    glGenFramebuffers(frameBuffers.size(), frameBuffers.data());

    // Setup multisampling framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, MSFBO);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, MSColorRenderBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, MSDepthStencilRenderBuffer);

    // Setup blit framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, blitFBO);
    for (int i = 0; i < TAASamples; i++){
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, frameTextureArray, 0, i);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, blitDepthStencilRenderBuffer);

    // Setup postprocess framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, PPFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fullResPPTextures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fullResPPTextures[1], 0);

    // Setup quarter res framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, QRFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, quarterResPPTextures[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, quarterResPPTextures[1], 0);

    {
      auto cubeVertexShaderSource = loadShaderSource("assets/shaders/triangle.vert");
      auto cubeFragmentShaderSource = loadShaderSource("assets/shaders/triangle.frag");
      auto lampVertexShaderSource = loadShaderSource("assets/shaders/lamp.vert");
      auto lampFragmentShaderSource = loadShaderSource("assets/shaders/lamp.frag");
      auto screenRectVertexShaderSource = loadShaderSource("assets/shaders/screenrect.vert");
      auto screenRectFragmentShaderSource = loadShaderSource("assets/shaders/screenrect.frag");
      auto TAAFragmentShaderSource = loadShaderSource("assets/shaders/taa.frag");
      auto greyscaleFragmentShaderSource = loadShaderSource("assets/shaders/greyscale.frag");
      auto bloomExtractFragmentShaderSource = loadShaderSource("assets/shaders/bloomextract.frag");
      auto bloomCombineFragmentShaderSource = loadShaderSource("assets/shaders/bloomcombine.frag");
      auto blurFragmentShaderSource = loadShaderSource("assets/shaders/blur.frag");
      auto cubeMapVertexShaderSource = loadShaderSource("assets/shaders/cube.vert");
      auto cubeMapFragmentShaderSource = loadShaderSource("assets/shaders/cube.frag");

      // Create cube shader program

      GLuint cubeVertexShader = createShader(GL_VERTEX_SHADER, cubeVertexShaderSource);
      GLuint cubeFragmentShader = createShader(GL_FRAGMENT_SHADER, cubeFragmentShaderSource);
      cubeShaderProgram = createProgram({cubeVertexShader, cubeFragmentShader});
      glDeleteShader(cubeVertexShader);
      glDeleteShader(cubeFragmentShader);

      // Create lamp shader program

      GLuint lampVertexShader = createShader(GL_VERTEX_SHADER, lampVertexShaderSource);
      GLuint lampFragmentShader = createShader(GL_FRAGMENT_SHADER, lampFragmentShaderSource);
      lampShaderProgram = createProgram({lampVertexShader, lampFragmentShader});
      glDeleteShader(lampVertexShader);
      glDeleteShader(lampFragmentShader);

      GLuint cubeMapVertexShader = createShader(GL_VERTEX_SHADER, cubeMapVertexShaderSource);
      GLuint cubeMapFragmentShader = createShader(GL_FRAGMENT_SHADER, cubeMapFragmentShaderSource);
      cubeMapShaderProgram = createProgram({cubeMapVertexShader, cubeMapFragmentShader});
      glDeleteShader(cubeMapVertexShader);
      glDeleteShader(cubeMapFragmentShader);

      // Create screen rect shader program

      GLuint screenRectVertexShader = createShader(GL_VERTEX_SHADER, screenRectVertexShaderSource);
      GLuint screenRectFragmentShader = createShader(GL_FRAGMENT_SHADER, screenRectFragmentShaderSource);
      screenRectShaderProgram = createProgram({screenRectVertexShader, screenRectFragmentShader});
      glDeleteShader(screenRectFragmentShader);

      // Create taa shader program

      GLuint TAAFragmentShader = createShader(GL_FRAGMENT_SHADER, TAAFragmentShaderSource);
      TAAShaderProgram = createProgram({screenRectVertexShader, TAAFragmentShader});
      glDeleteShader(TAAFragmentShader);

      // Create greyscale shader program

      GLuint greyscaleFragmentShader = createShader(GL_FRAGMENT_SHADER, greyscaleFragmentShaderSource);
      greyscaleShaderProgram = createProgram({screenRectVertexShader, greyscaleFragmentShader});
      glDeleteShader(greyscaleFragmentShader);

      // Create bloom shader programs

      GLuint bloomExtractFragmentShader = createShader(GL_FRAGMENT_SHADER, bloomExtractFragmentShaderSource);
      bloomExtractShaderProgram = createProgram({screenRectVertexShader, bloomExtractFragmentShader});
      glDeleteShader(bloomExtractFragmentShader);

      GLuint bloomCombineFragmentShader = createShader(GL_FRAGMENT_SHADER, bloomCombineFragmentShaderSource);
      bloomCombineShaderProgram = createProgram({screenRectVertexShader, bloomCombineFragmentShader});
      glDeleteShader(bloomCombineFragmentShader);

      // Create blur shader program

      GLuint blurFragmentShader = createShader(GL_FRAGMENT_SHADER, blurFragmentShaderSource);
      blurShaderProgram = createProgram({screenRectVertexShader, blurFragmentShader});
      glDeleteShader(blurFragmentShader);

      glDeleteShader(screenRectVertexShader);
    }

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
    storeData(screenRectVertexData, rectVertexIndices, screenRectVBO, screenRectEBO);
    storeData(magRectVertexData, rectVertexIndices, magRectVBO, 0);
    storeData(transparantObjectVertexData, transparentObjectVertexIndices, transparentVBO, transparentEBO);
    storeData(skyboxVertexData, skyboxVertexIndices, skyboxVBO, skyboxEBO);

    // Setup VAOs

    glGenVertexArrays(vertexArrays.size(), vertexArrays.data());
    setupModel(cubeVAO, cubeVBO, cubeEBO);
    setupModel(pyramidVAO, pyramidVBO, pyramidEBO);
    setupModel(floorVAO, circlePlaneVBO, planeEBO);
    setupLamp(pointLightVAO, sphereVBO, sphereEBO);
    setupLamp(spotLightVAO, coneVBO, coneEBO);
    setupLamp(directionalLightVAO, squarePlaneVBO, squarePlaneEBO);
    setupRenderRect(screenRectVAO, screenRectVBO, screenRectEBO);
    setupRenderRect(magRectVAO, magRectVBO, screenRectEBO);
    setupModel(transparentVAO, transparentVBO, transparentEBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (decltype(skyboxVertexData)::value_type), nullptr);
    glEnableVertexAttribArray(0);

    // Set floor texture wrapping parameters

    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(circularPlaneMaterial.diffuseMap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(circularPlaneMaterial.specularMap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenBuffers(uniformBuffers.size(), uniformBuffers.data());

    glBindBuffer(GL_UNIFORM_BUFFER, matrixUBO);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof (glm::mat4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matrixUBO);

    glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
    glBufferData(GL_UNIFORM_BUFFER, 2576, nullptr, GL_DYNAMIC_DRAW);
    std::vector<std::byte> bufferData(2576);
    auto bufferPointer = bufferData.data();
    for (int i = 0; i < pointLights.size(); i++){
        int offset = 80 * i;
        const auto& light = pointLights[i];
        copyLightColor(bufferPointer + offset, light);
        copyLightAttenuation(bufferPointer + offset + 48, light);
        copyLightPosition(bufferPointer + offset + 64, light);
    }
    for (int i = 0; i < spotLights.size(); i++){
        int offset = 80 * MAX_POINT_LIGHTS + 112 * i;
        const auto& light = spotLights[i];
        copyLightColor(bufferPointer + offset, light);
        copyLightAttenuation(bufferPointer + offset + 48, light);
        copyLightPosition(bufferPointer + offset + 64, light);
        copyLightDirection(bufferPointer + offset + 80, light);
        copyLightAngle(bufferPointer + offset + 92, light);
    }
    for (int i = 0; i < directionalLights.size(); i++){
        int offset = 80 * MAX_POINT_LIGHTS + 112 * MAX_SPOT_LIGHTS + 64 * i;
        const auto& light = directionalLights[i];
        copyLightColor(bufferPointer + offset, light);
        copyLightDirection(bufferPointer + offset + 48, light);
    }
    {
        int baseOffset = 80 * MAX_POINT_LIGHTS + 112 * MAX_SPOT_LIGHTS + 64 * MAX_DIRECTIONAL_LIGHTS;
        std::memcpy(bufferPointer + baseOffset + 0, &numPointLights, 4);
        std::memcpy(bufferPointer + baseOffset + 8, &numDirectionalLights, 4);
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 2576, bufferData.data());
    bufferData.clear();
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, lightUBO);

    glUseProgram(cubeShaderProgram);
    glUniformBlockBinding(cubeShaderProgram, glGetUniformBlockIndex(cubeShaderProgram, "MatrixBlock"), 0);
    glUniformBlockBinding(cubeShaderProgram, glGetUniformBlockIndex(cubeShaderProgram, "LightsBlock"), 1);
    glUniform1i(glGetUniformLocation(cubeShaderProgram, "material.diffuse"), 0);
    glUniform1i(glGetUniformLocation(cubeShaderProgram, "material.specular"), 1);

    glUseProgram(lampShaderProgram);
    glUniformBlockBinding(lampShaderProgram, glGetUniformBlockIndex(lampShaderProgram, "MatrixBlock"), 0);

    glUseProgram(TAAShaderProgram);
    glUniform1i(glGetUniformLocation(TAAShaderProgram, "frames"), 0);
    glUniform1i(glGetUniformLocation(TAAShaderProgram, "numFrames"), TAASamples);

    glUseProgram(greyscaleShaderProgram);
    glUniform1i(glGetUniformLocation(greyscaleShaderProgram, "inputFrame"), 0);

    glUseProgram(bloomExtractShaderProgram);
    glUniform1i(glGetUniformLocation(bloomExtractShaderProgram, "inputFrame"), 0);
    glUniform1f(glGetUniformLocation(bloomExtractShaderProgram, "intencity"), bloomIntencity);

    glUseProgram(bloomCombineShaderProgram);
    glUniform1i(glGetUniformLocation(bloomCombineShaderProgram, "baseFrame"), 0);
    glUniform1i(glGetUniformLocation(bloomCombineShaderProgram, "bloomFrame"), 1);

    glUseProgram(blurShaderProgram);
    glUniform1i(glGetUniformLocation(blurShaderProgram, "inputFrame"), 0);

    glUseProgram(cubeMapShaderProgram);
    glUniform1i(glGetUniformLocation(cubeMapShaderProgram, "cubeMap"), 0);

    float forwardAxisValue, rightAxisValue, upAxisValue;

    float previousTime = 0.0f;

    auto window = CameraManager::getWindow();

    while (not glfwWindowShouldClose(window)){
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - previousTime;

        forwardAxisValue = 0.0f;
        rightAxisValue = 0.0f;
        upAxisValue = 0.0f;
        bFlashLight = false;
        bGreyScale = false;

        // Input

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS){
            bTAA = false;
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS){
            bTAA = true;
        }
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS){
            bMSAA = false;
        }
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS){
            bMSAA = true;
        }
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS){
            bBloom = false;
        }
        if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS){
            bBloom = true;
        }
        if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS){
            bSkybox = false;
        }
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS){
            bSkybox = true;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
            CameraManager::disableCameraLook();
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
            CameraManager::enableCameraLook();
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
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS){
            bGreyScale = true;
        }
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS){
            bShowMag = true;
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS){
            bShowMag = false;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){
            bFlashLight = true;
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

        glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
        if (bFlashLight and numSpotLights > 0){
            spotLights.back().direction = camera->getCameraForwardVector();
            spotLights.back().position = camera->getCameraPos();
            int offset = 80 * MAX_POINT_LIGHTS + 112 * (spotLights.size() - 1);
            glBufferSubData(GL_UNIFORM_BUFFER, offset + 64, 12, glm::value_ptr(spotLights.back().position));
            glBufferSubData(GL_UNIFORM_BUFFER, offset + 80, 12, glm::value_ptr(spotLights.back().direction));
        }
        {
            int numUsedSpotlights = std::max(numSpotLights - !bFlashLight, 0);
            int offset = 80 * MAX_POINT_LIGHTS + 112 * MAX_SPOT_LIGHTS + 64 * MAX_DIRECTIONAL_LIGHTS + 4;
            glBufferSubData(GL_UNIFORM_BUFFER, offset, 4, &numUsedSpotlights);
        }

        glm::mat4 view = CameraManager::getViewMatrix(),
                  projection = CameraManager::getProjectionMatrix();
        if (bTAA){
            glm::vec3 sampleTrans = TAASamplesPositions[colorIndex];
            sampleTrans.x /= windowW;
            sampleTrans.y /= windowH;
            projection = glm::translate(glm::mat4(1.0f), sampleTrans) * projection;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, blitFBO);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + colorIndex);
        if (bMSAA){
            glBindFramebuffer(GL_FRAMEBUFFER, MSFBO);
        }
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindBuffer(GL_UNIFORM_BUFFER, matrixUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof (glm::mat4), glm::value_ptr(view));

        // Setup model draw parameters

        glEnable(GL_CULL_FACE);

        glUseProgram(cubeShaderProgram);

        glUniform3fv(glGetUniformLocation(cubeShaderProgram, "cameraPos"), 1, glm::value_ptr(camera->getCameraPos()));

        // Draw cubes

        setShaderMatrial(cubeShaderProgram, cubeMaterial);

        for (const auto& [m, n] : cubeMatrices){
            setModelUniforms(cubeShaderProgram, m, n);
            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, cubeVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        // Draw pyramids

        setShaderMatrial(cubeShaderProgram, pyramidMaterial);

        for (const auto& [m, n] : pyramidMatrices){
            setModelUniforms(cubeShaderProgram, m, n);
            glBindVertexArray(pyramidVAO);
            glDrawElements(GL_TRIANGLES, pyramidVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        // Draw floor

        glDisable(GL_CULL_FACE);

        setShaderMatrial(cubeShaderProgram, circularPlaneMaterial);

        setModelUniforms(cubeShaderProgram, floorModel, floorNormal);
        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, circularPlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);

        glEnable(GL_CULL_FACE);

        // Draw lamps

        glUseProgram(lampShaderProgram);

        for (int i = 0; i < numPointLights; i++){
            setLampUniforms(lampShaderProgram, pointLightMatrices[i], pointLights[i].diffuse);
            glBindVertexArray(pointLightVAO);
            glDrawElements(GL_TRIANGLES, sphereVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        for (int i = 0; i < numSpotLights - 1; i++){
            setLampUniforms(lampShaderProgram, spotLightMatrices[i], spotLights[i].diffuse);
            glBindVertexArray(spotLightVAO);
            glDrawElements(GL_TRIANGLES, coneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glDisable(GL_CULL_FACE);

        for (int i = 0; i < numDirectionalLights; i++){
            setLampUniforms(lampShaderProgram, directionalLightMatrices[i], directionalLights[i].diffuse);
            glBindVertexArray(directionalLightVAO);
            glDrawElements(GL_TRIANGLES, squarePlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glEnable(GL_CULL_FACE);

        // Draw skybox

        if (bSkybox){
            view = glm::mat4(glm::mat3(view));
            glUseProgram(cubeMapShaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(cubeMapShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(cubeMapShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, TextureLoader::getTextureIdCubeMap(cubeMapFaceTextures));
            glBindVertexArray(skyboxVAO);
            glDrawElements(GL_TRIANGLES, skyboxVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        // Draw transparent objects

        glEnable(GL_BLEND);

        glUseProgram(cubeShaderProgram);

        std::sort(transparentObjects.begin(), transparentObjects.end(),
                  [&camera] (const std::tuple<glm::mat4, glm::mat3, Material>& rhs, const std::tuple<glm::mat4, glm::mat3, Material>& lhs){
                        auto cameraPos = camera->getCameraPos();
                        auto rhs_pos = glm::vec3(std::get<0>(rhs)[3]);
                        auto lhs_pos = glm::vec3(std::get<0>(lhs)[3]);
                        return glm::length(rhs_pos - cameraPos) > glm::length(lhs_pos - cameraPos);
                  }
        );

        for (const auto& [model, normal, material] : transparentObjects){
            setShaderMatrial(cubeShaderProgram, material);
            setModelUniforms(cubeShaderProgram, model, normal);
            glBindVertexArray(transparentVAO);
            glDrawElements(GL_TRIANGLES, transparentObjectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glDisable(GL_BLEND);

        // Blit MSAA framebuffer

        if (bMSAA){
            glBindFramebuffer(GL_READ_FRAMEBUFFER, MSFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blitFBO);
            glBlitFramebuffer(0, 0, windowW, windowH, 0, 0, windowW, windowH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }

        // Do postprocessing

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        // Setup initial input texture

        glBindFramebuffer(GL_FRAMEBUFFER, PPFBO);
        swapBuffers(fullResReadIndex);

        if (bTAA){
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, frameTextureArray);
            glUseProgram(TAAShaderProgram);
            glBindVertexArray(screenRectVAO);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }
        else {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, blitFBO);
            glReadBuffer(GL_COLOR_ATTACHMENT0 + colorIndex);
            glBlitFramebuffer(0, 0, windowW, windowH, 0, 0, windowW, windowH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, PPFBO);
        swapBuffers(fullResReadIndex);

        glBindVertexArray(screenRectVAO);
        glActiveTexture(GL_TEXTURE0);

        if (bBloom) {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, QRFBO);
            glViewport(0, 0, windowW / 2, windowH / 2);
            swapBuffers(quarterResReadIndex);
            glReadBuffer(GL_COLOR_ATTACHMENT0 + fullResReadIndex);
            glBlitFramebuffer(0, 0, windowW, windowH, 0, 0, windowW / 2, windowH / 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, QRFBO);
            swapBuffers(quarterResReadIndex);

            glUseProgram(bloomExtractShaderProgram);
            glBindTexture(GL_TEXTURE_2D, quarterResPPTextures[quarterResReadIndex]);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            swapBuffers(quarterResReadIndex);

            glUseProgram(blurShaderProgram);
            glUniform1f(glGetUniformLocation(blurShaderProgram, "strideX"), 2.0f / windowW);
            glUniform1f(glGetUniformLocation(blurShaderProgram, "strideY"), 2.0f / windowH);

            for (auto horizontal : {0, 1}){
                glUniform1i(glGetUniformLocation(blurShaderProgram, "bHorizontal"), horizontal);
                glBindTexture(GL_TEXTURE_2D, quarterResPPTextures[quarterResReadIndex]);
                glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
                swapBuffers(quarterResReadIndex);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, PPFBO);
            glViewport(0, 0, windowW, windowH);
            glUseProgram(bloomCombineShaderProgram);
            glBindTexture(GL_TEXTURE_2D, fullResPPTextures[fullResReadIndex]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, quarterResPPTextures[quarterResReadIndex]);
            glActiveTexture(GL_TEXTURE0);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            swapBuffers(fullResReadIndex);
        }

        if (bGreyScale){
            glUseProgram(greyscaleShaderProgram);
            glBindTexture(GL_TEXTURE_2D, fullResPPTextures[fullResReadIndex]);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            swapBuffers(fullResReadIndex);
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, windowW, windowH, 0, 0, windowW, windowH, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        if (bShowMag){
            glUseProgram(screenRectShaderProgram);
            glBindTexture(GL_TEXTURE_2D, fullResPPTextures[fullResReadIndex]);
            glBindVertexArray(magRectVAO);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glEnable(GL_DEPTH_TEST);

        colorIndex = (colorIndex + 1) % TAASamples;

        glfwSwapBuffers(window);
        glfwPollEvents();

        previousTime = currentTime;
    }

    CameraManager::terminate();

    return 0;
}
