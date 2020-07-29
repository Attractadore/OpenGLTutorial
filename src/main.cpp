#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "Lights.hpp"
#include "RandomSampler.hpp"
#include "TextureLoader.hpp"

#include <GLFW/glfw3.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <regex>

struct Material {
    std::string diffuseMap;
    std::string specularMap;
    float shininess;
};

bool operator==(const Material& m1, const Material& m2) {
    return m1.diffuseMap == m2.diffuseMap and m1.specularMap == m2.specularMap and m1.shininess == m2.shininess;
}

bool operator!=(const Material& m1, const Material& m2) {
    return m1.diffuseMap != m2.diffuseMap or m1.specularMap != m2.specularMap or m1.shininess != m2.shininess;
}

struct MeshData {
    std::vector<GLfloat> vertexData;
    std::vector<GLuint> vertexIndices;
    Material meshMaterial;
};

void debugFunction(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::printf("%s\n", message);
    }
}

std::string loadShaderSource(std::filesystem::path filePath) {
    std::ifstream is(filePath, std::ios::binary);
    if (!is) {
        return "";
    }
    auto fileSize = std::filesystem::file_size(filePath);
    std::string fileContents(fileSize, ' ');
    is.read(fileContents.data(), fileSize);
    std::regex includeRegex(R"(#include *"(.+)\")");
    std::smatch regex_matches;
    while (std::regex_search(fileContents, regex_matches, includeRegex)) {
        fileContents.replace(regex_matches[0].first, regex_matches[0].second, loadShaderSource("assets/shaders/" + regex_matches[1].str()));
    }
    return fileContents;
}

GLuint createShader(GLenum shaderType, const std::string& shaderSource) {
    const char* shaderSourceCStr = shaderSource.c_str();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSourceCStr, nullptr);
    glCompileShader(shader);
    return shader;
}

GLuint createProgram(const std::vector<GLuint>& shaders) {
    GLuint program = glCreateProgram();
    for (auto& shader: shaders) {
        glAttachShader(program, shader);
    }
    glLinkProgram(program);
    for (auto& shader: shaders) {
        glDetachShader(program, shader);
    }
    return program;
}

template <typename S1, typename S2>
void storeData(const S1& vertexData, const S2& vertexIndices, GLuint VBO, GLuint EBO) {
    static_assert(std::is_same_v<typename S1::value_type, GLfloat>);
    static_assert(std::is_same_v<typename S2::value_type, GLuint>);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);
}

void setupModel(GLuint VAO, GLuint VBO, GLuint EBO) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
}

void setupLamp(GLuint VAO, GLuint VBO, GLuint EBO) {
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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void*>(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}

void setShaderMatrial(GLuint shaderProgram, const Material& material) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(material.diffuseMap));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(material.specularMap, false));
    glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), material.shininess);
}

void setModelUniforms(GLuint shaderProgram, const glm::mat4& model, const glm::mat3& normal) {
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normal"), 1, GL_FALSE, glm::value_ptr(normal));
}

void setLampUniforms(GLuint shaderProgram, const glm::mat4& model, const glm::vec3& lightColor) {
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
}

std::vector<MeshData> loadModelData(const std::string& modelPath) {
    if (!std::filesystem::exists(modelPath)) {
        return {};
    }
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath.c_str(), aiProcess_JoinIdenticalVertices | aiProcess_Triangulate);
    if (!scene) {
        return {};
    }
    std::vector<MeshData> meshes;
    for (std::size_t i = 0; i < scene->mNumMeshes; i++) {
        MeshData newMeshData;
        auto mesh = scene->mMeshes[i];
        auto meshMaterial = scene->mMaterials[mesh->mMaterialIndex];
        for (std::size_t j = 0; j < mesh->mNumVertices; j++) {
            newMeshData.vertexData.push_back(mesh->mVertices[j].x);
            newMeshData.vertexData.push_back(mesh->mVertices[j].y);
            newMeshData.vertexData.push_back(mesh->mVertices[j].z);
            if (mesh->HasNormals()) {
                newMeshData.vertexData.push_back(mesh->mNormals[j].x);
                newMeshData.vertexData.push_back(mesh->mNormals[j].y);
                newMeshData.vertexData.push_back(mesh->mNormals[j].z);
            } else {
                newMeshData.vertexData.push_back(0.0f);
                newMeshData.vertexData.push_back(0.0f);
                newMeshData.vertexData.push_back(0.0f);
            }
            if (mesh->HasTextureCoords(0)) {
                newMeshData.vertexData.push_back(mesh->mTextureCoords[0][j].x);
                newMeshData.vertexData.push_back(mesh->mTextureCoords[0][j].y);
            } else {
                newMeshData.vertexData.push_back(0.0f);
                newMeshData.vertexData.push_back(0.0f);
            }
        }
        for (std::size_t j = 0; j < mesh->mNumFaces; j++) {
            auto face = mesh->mFaces[j];
            for (std::size_t k = 0; k < face.mNumIndices; k++) {
                newMeshData.vertexIndices.push_back(face.mIndices[k]);
            }
        }
        aiString diffuseTexture;
        aiString specularTexture;
        meshMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexture);
        meshMaterial->GetTexture(aiTextureType_SPECULAR, 0, &specularTexture);
        newMeshData.meshMaterial.diffuseMap = diffuseTexture.C_Str();
        newMeshData.meshMaterial.specularMap = specularTexture.C_Str();
        newMeshData.meshMaterial.shininess = 128.0f;
        meshes.push_back(std::move(newMeshData));
    }
    return meshes;
}

void copyLightColor(std::byte* dst, const LightCommon& light) {
    std::memcpy(dst + 00, glm::value_ptr(light.ambient), 12);
    std::memcpy(dst + 16, glm::value_ptr(light.diffuse), 12);
    std::memcpy(dst + 32, glm::value_ptr(light.specular), 12);
}

void copyLightRadius(std::byte* dst, const PointLight& light) {
    std::memcpy(dst, &light.radius, 4);
}

void copyLightPosition(std::byte* dst, const PointLight& light) {
    std::memcpy(dst, glm::value_ptr(light.position), 12);
}

void copyLightDirection(std::byte* dst, const DirectionalLight& light) {
    std::memcpy(dst, glm::value_ptr(light.direction), 12);
}

void copyLightAngle(std::byte* dst, const SpotLight& light) {
    std::memcpy(dst + 0, &light.innerAngleCos, 4);
    std::memcpy(dst + 4, &light.outerAngleCos, 4);
}

void calculatePyramidMatrices(int w, int h, float dX, float dY, std::vector<std::pair<glm::mat4, glm::mat3>>& matrices) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if ((i + j) % 3) {
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

void calculateCubeMatrices(int w, int h, float dX, float dY, std::vector<std::pair<glm::mat4, glm::mat3>>& matrices) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            if (!((i + j) % 3)) {
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

void calculatePointLightMatrices(const std::vector<PointLight>& lights, std::vector<glm::mat4>& matrices) {
    for (const auto& light: lights) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
        model = glm::scale(model, light.radius * glm::vec3(1.0f, 1.0f, 1.0f));
        matrices.push_back(model);
    }
}

void calculateSpotLightMatrices(const std::vector<SpotLight>& lights, std::vector<glm::mat4>& matrices) {
    for (const auto& light: lights) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
        float angle = glm::acos(glm::dot(glm::normalize(light.direction), {0.0f, 0.0f, -1.0f}));
        glm::vec3 rotateAxis = glm::normalize(glm::cross({0.0f, 0.0f, -1.0f}, light.direction));
        if (glm::length(rotateAxis) > 0.0f) {
            model = glm::rotate(model, angle, rotateAxis);
        }
        model = glm::scale(model, light.radius / glm::sqrt(3.0f) * glm::vec3(1.0f, 1.0f, 1.0f));
        matrices.push_back(model);
    }
}

void calculateDirectionalLightMatrices(const std::vector<DirectionalLight>& lights, std::vector<glm::mat4>& matrices) {
    for (const auto& light: lights) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 30.0f});
        float angle = glm::acos(glm::dot(glm::normalize(light.direction), {0.0f, 0.0f, -1.0f}));
        glm::vec3 rotateAxis = glm::cross({0.0f, 0.0f, -1.0f}, light.direction);
        if (glm::length(rotateAxis) > 0.0f) {
            rotateAxis = glm::normalize(rotateAxis);
            model = glm::rotate(model, angle, rotateAxis);
        }
        matrices.push_back(model);
    }
}

void setupWindows(std::vector<std::tuple<glm::mat4, glm::mat3, Material>>& windows, float distX, float distY, const Material& windowMaterial) {
    constexpr std::array<glm::vec3, 4> points = {
        glm::vec3{-1.0f, 1.0f, 0.0f},
        glm::vec3{1.0f, 1.0f, 0.0f},
        glm::vec3{1.0f, -1.0f, 0.0f},
        glm::vec3{-1.0f, -1.0f, 0.0f},
    };
    for (int i = 0; i < 4; i++) {
        glm::vec3 dir = glm::normalize(points[(i + 1) % 4] - points[i]);
        float dist;
        if (i % 1) {
            dist = distY;
        } else {
            dist = distX;
        }
        glm::vec3 start = points[i] * dist;
        start += dir * (1.0f + std::remainder(2.0f * dist, 2.0f));
        glm::mat4 model(1.0f);
        if (i % 2) {
            model = glm::rotate(model, glm::radians(90.0f), {0.0f, 0.0f, 1.0f});
        }
        glm::mat3 normal = glm::mat3(model);
        for (int j = 0; j < std::floor(dist); j++) {
            glm::mat4 windowModel = glm::translate(glm::mat4(1.0f), start) * model;
            windows.emplace_back(windowModel, normal, windowMaterial);
            start += 2.0f * dir;
        }
    }
}

void setupGrass(std::vector<std::tuple<glm::mat4, glm::mat3, Material>>& grass, float distX, float distY, const Material& grassMaterial) {
    float radius = std::sqrt(distX * distX + distY * distY) + 1.0f;
    int numGrassTufts = std::floor(glm::radians(360.0f) * radius);
    float angleStep = glm::radians(360.0f) / numGrassTufts;
    glm::vec3 dir = {0.0f, 1.0f, 0.0f};
    for (int i = 0; i < numGrassTufts; i++) {
        float angle = angleStep * i;
        glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), angle, {0.0f, 0.0f, 1.0f});
        glm::mat4 transMat = glm::translate(glm::mat4(1.0f), glm::mat3(rotMat) * dir * radius);
        glm::mat4 model = transMat * rotMat;
        glm::mat3 normal = model;
        grass.emplace_back(model, normal, grassMaterial);
    }
}

void setupSnowData(GLuint snowPosVBO, GLuint snowDirVBO, int numSnowParticles, float xSpan, float ySpan, float zStart, float zFloor) {
    std::vector<GLfloat> particlesStartingPositions, particlesFallVectors;
    for (int i = 0; i < numSnowParticles; i++) {
        particlesStartingPositions.push_back(RandomSampler::randomFloat(-xSpan, xSpan));
        particlesStartingPositions.push_back(RandomSampler::randomFloat(-ySpan, ySpan));
        particlesStartingPositions.push_back(zStart);
        particlesFallVectors.push_back(0.0f);
        particlesFallVectors.push_back(0.0f);
        particlesFallVectors.push_back(zFloor - zStart);
    }
    glBindBuffer(GL_ARRAY_BUFFER, snowPosVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * particlesStartingPositions.size(), particlesStartingPositions.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, snowDirVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * particlesFallVectors.size(), particlesFallVectors.data(), GL_STATIC_DRAW);
}

void swapBuffers(int& readBufferIndex) {
    readBufferIndex = !readBufferIndex;
    glReadBuffer(GL_COLOR_ATTACHMENT0 + readBufferIndex);
    glDrawBuffer(GL_COLOR_ATTACHMENT0 + !readBufferIndex);
}

void drawShadowCasters(GLuint shadowProgram, const std::vector<glm::mat4>& lightTransforms, GLuint cubeVAO, const std::vector<std::pair<glm::mat4, glm::mat3>>& cubeMatrices, int numCubeVertices, GLuint pyramidVAO, const std::vector<std::pair<glm::mat4, glm::mat3>>& pyramidMatrices, int numPyramidVertices) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glUseProgram(shadowProgram);

    glUniform1i(glGetUniformLocation(shadowProgram, "numLayers"), lightTransforms.size());
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "lightTransforms"), lightTransforms.size(), GL_FALSE, reinterpret_cast<const GLfloat*>(lightTransforms.data()));

    for (const auto& [m, _]: cubeMatrices) {
        glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, numCubeVertices, GL_UNSIGNED_INT, nullptr);
    }

    for (const auto& [m, _]: pyramidMatrices) {
        glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
        glBindVertexArray(pyramidVAO);
        glDrawElements(GL_TRIANGLES, numPyramidVertices, GL_UNSIGNED_INT, nullptr);
    }

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
}

glm::vec3 calculateLightUp(const glm::vec3& lightDir) {
    glm::vec3 lightUp = {0.0f, 0.0f, 1.0f};
    glm::vec3 lightLeft = glm::cross(lightUp, lightDir);
    if (glm::length(lightLeft) > 0.0f) {
        lightUp = glm::cross(lightDir, lightLeft);
    } else {
        lightUp = {0.0f, 1.0f, 0.0f};
    }
    return lightUp;
}

int main() {
    float horizontalFOV = 90.0f;
    int windowW, windowH;
    float windowAspectRatio = 16.0f / 9.0f;
    windowH = 720.0f;
    windowW = windowH * windowAspectRatio;
    float cameraSpeed = 5.0f;

    constexpr int MAX_POINT_LIGHTS = 10,
                  MAX_SPOT_LIGHTS = 10,
                  MAX_DIRECTIONAL_LIGHTS = 10;
    constexpr int POINT_LIGHT_SIZE = 64,
                  SPOT_LIGHT_SIZE = 96,
                  DIRECTIONAL_LIGHT_SIZE = 64;

    int numPointLights = 3,
        numSpotLights = 3 + 1,
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
         bSkybox = false,
         bBorder = false,
         bSnow = false,
         bShowMag = false,
         bGammaCorrect = true;
    float bloomIntencity = 16.0f;
    int TAASamples = 4;
    std::vector<glm::vec3> TAASamplesPositions =
        {{-0.25f, -0.25f, 0.0f},
         {0.25f, -0.25f, 0.0f},
         {0.25f, 0.25f, 0.0f},
         {-0.25f, 0.25f, 0.0f}};
    int MSAASamples = 4;
    float gammaValue = 2.2f;
    constexpr int SHADOWMAP_RESOLUTION = 1024;
    constexpr int DIR_LIGHT_SHADOWMAP_RESOLUTION = SHADOWMAP_RESOLUTION * 8;

    int numCubesX = 10,
        numCubesY = numCubesX;
    float cubeDistanceX = 4.0f,
          cubeDistanceY = cubeDistanceX;

    std::vector<std::pair<glm::mat4, glm::mat3>> pyramidMatrices,
        cubeMatrices;

    int numSnowParticles = 10'000;
    glm::mat4 snowParticleModel = glm::scale(glm::mat4(1.0f), 0.02f * glm::vec3(1.0f));
    std::vector<GLuint> snowTextures(2);
    GLuint& snowDiffuseTexture = snowTextures[0];
    GLuint& snowSpecularTexture = snowTextures[1];

    std::vector<glm::mat4> pointLightMatrices, spotLightMatrices, directionalLightMatrices;

    glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -1.01f}),
              floorNormal(1.0f);

    calculatePyramidMatrices(numCubesX, numCubesY, cubeDistanceX, cubeDistanceY, pyramidMatrices);
    calculateCubeMatrices(numCubesX, numCubesY, cubeDistanceX, cubeDistanceY, cubeMatrices);
    calculatePointLightMatrices(pointLights, pointLightMatrices);
    calculateSpotLightMatrices(spotLights, spotLightMatrices);
    calculateDirectionalLightMatrices(directionalLights, directionalLightMatrices);

    Material windowMaterial = {"window.png", "window_specular.png", 256.0f},
             grassMaterial = {"grass.png", "grass_specular.png", 64.0f};
    std::vector<std::string> cubeMapFaceTextures = {
        "skybox/front.png",
        "skybox/back.png",
        "skybox/right.png",
        "skybox/left.png",
        "skybox/top.png",
        "skybox/bottom.png"};

    std::vector<std::tuple<glm::mat4, glm::mat3, Material>> transparentObjects;
    float windowRectDW = numCubesX * cubeDistanceX / 2.0f + 2.0f,
          windowRectDH = numCubesY * cubeDistanceY / 2.0f + 2.0f;
    setupWindows(transparentObjects, windowRectDW, windowRectDH, windowMaterial);
    setupGrass(transparentObjects, windowRectDW, windowRectDH, grassMaterial);

    glm::vec3 cameraStartPos = {0.0f, 0.0f, 3.0f},
              cameraStartLookDirection = {1.0f, 0.0f, 0.0f};

    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDirection, glm::vec3(0.0f, 0.0f, 1.0f));

    GLuint cubeShaderProgram,
        cubeNormalShaderProgram,
        lampShaderProgram,
        lampBorderShaderProgram,
        TAAShaderProgram,
        greyscaleShaderProgram,
        gammaCorrectionShaderProgram,
        bloomExtractShaderProgram,
        bloomCombineShaderProgram,
        screenRectShaderProgram,
        blurShaderProgram,
        cubeMapShaderProgram,
        snowShaderProgram,
        shadowShaderProgram,
        depthVisualizationProgram;
    GLuint frameTextureArray;
    std::vector<GLuint> shadowMapArrays(2);
    GLuint& spotLightShadowMapArray = shadowMapArrays[0];
    GLuint& directionalLightShadowMapArray = shadowMapArrays[1];
    GLuint pointLightShadowCubeMapArray;
    std::vector<GLuint> fullResPPTextures(2),
        quarterResPPTextures(2);
    int colorIndex = 0,
        fullResReadIndex = 0,
        quarterResReadIndex = 0;

    std::vector<GLuint> vertexBuffers(12);

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
    GLuint& snowPosVBO = vertexBuffers[10];
    GLuint& snowDirVBO = vertexBuffers[11];

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

    std::vector<GLuint> frameBuffers(5);
    GLuint& PPFBO = frameBuffers[0];
    GLuint& MSFBO = frameBuffers[1];
    GLuint& blitFBO = frameBuffers[2];
    GLuint& QRFBO = frameBuffers[3];
    GLuint& shadowMapFBO = frameBuffers[4];

    std::vector<GLuint> renderBuffers(3);
    GLuint& MSColorRenderBuffer = renderBuffers[0];
    GLuint& MSDepthStencilRenderBuffer = renderBuffers[1];
    GLuint& blitDepthStencilRenderBuffer = renderBuffers[2];

    std::vector<GLuint> vertexArrays(11);

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
    GLuint& snowVAO = vertexArrays[10];

    constexpr std::array<GLfloat, 16> screenRectVertexData =
        {-1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f, 1.0f, 1.0f, 1.0f,
         -1.0f, 1.0f, 0.0f, 1.0f};

    constexpr std::array<GLfloat, 16> magRectVertexData =
        {-1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.125f, 0.0f,
         0.0f, 1.0f, 0.125f, 0.125f,
         -1.0f, 1.0f, 0.0f, 0.125f};

    constexpr std::array<GLuint, 6> rectVertexIndices =
        {0, 1, 2,
         2, 3, 0};

    CameraManager::initialize(windowW, windowH);
    CameraManager::setHorizontalFOV(horizontalFOV);
    CameraManager::currentCamera = camera;
    CameraManager::enableCameraLook();
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    if (glIsEnabled(GL_DEBUG_OUTPUT)) {
        glDebugMessageCallback(debugFunction, nullptr);
    }
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup snow material
    glGenTextures(snowTextures.size(), snowTextures.data());
    for (const auto& tex: snowTextures) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, glm::value_ptr(glm::vec4(1.0f)));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Setup rendering textures

    glGenTextures(fullResPPTextures.size(), fullResPPTextures.data());
    for (const auto& tex: fullResPPTextures) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, windowW, windowH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glGenTextures(quarterResPPTextures.size(), quarterResPPTextures.data());
    for (const auto& tex: quarterResPPTextures) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, windowW / 2, windowH / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glGenTextures(1, &frameTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, frameTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16, windowW, windowH, TAASamples, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(shadowMapArrays.size(), shadowMapArrays.data());
    glBindTexture(GL_TEXTURE_2D_ARRAY, spotLightShadowMapArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, numSpotLights, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D_ARRAY, directionalLightShadowMapArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, DIR_LIGHT_SHADOWMAP_RESOLUTION, DIR_LIGHT_SHADOWMAP_RESOLUTION, numDirectionalLights, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    for (auto shadowTex : shadowMapArrays) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowTex);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(glm::vec4(1.0f)));
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }

    glGenTextures(1, &pointLightShadowCubeMapArray);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, pointLightShadowCubeMapArray);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0, GL_DEPTH_COMPONENT, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 6 * numPointLights, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // Setup renderbuffers

    int maxSamples;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    MSAASamples = std::min(MSAASamples, maxSamples);
    glGenRenderbuffers(renderBuffers.size(), renderBuffers.data());
    glBindRenderbuffer(GL_RENDERBUFFER, MSColorRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAASamples, GL_RGBA16, windowW, windowH);
    glBindRenderbuffer(GL_RENDERBUFFER, MSDepthStencilRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAASamples, GL_DEPTH24_STENCIL8, windowW, windowH);
    glBindRenderbuffer(GL_RENDERBUFFER, blitDepthStencilRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windowW, windowH);

    // Setup framebuffers

    glGenFramebuffers(frameBuffers.size(), frameBuffers.data());

    // Setup multisampling framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, MSFBO);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, MSColorRenderBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, MSDepthStencilRenderBuffer);

    // Setup blit framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, blitFBO);
    for (int i = 0; i < TAASamples; i++) {
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

    // Setup shadow map framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    {
        auto cubeVertexShaderSource = loadShaderSource("assets/shaders/triangle.vert");
        auto cubeFragmentShaderSource = loadShaderSource("assets/shaders/triangle.frag");
        auto cubeNormalVertexShaderSource = loadShaderSource("assets/shaders/trianglenormals.vert");
        auto cubeNormalGeometryShaderSource = loadShaderSource("assets/shaders/trianglenormals.geom");
        auto cubeNormalFragmentShaderSource = loadShaderSource("assets/shaders/trianglenormals.frag");
        auto lampVertexShaderSource = loadShaderSource("assets/shaders/lamp.vert");
        auto lampFragmentShaderSource = loadShaderSource("assets/shaders/lamp.frag");
        auto lampBorderFragmentShaderSource = loadShaderSource("assets/shaders/lampborder.frag");
        auto screenRectVertexShaderSource = loadShaderSource("assets/shaders/screenrect.vert");
        auto screenRectFragmentShaderSource = loadShaderSource("assets/shaders/screenrect.frag");
        auto TAAFragmentShaderSource = loadShaderSource("assets/shaders/taa.frag");
        auto greyscaleFragmentShaderSource = loadShaderSource("assets/shaders/greyscale.frag");
        auto bloomExtractFragmentShaderSource = loadShaderSource("assets/shaders/bloomextract.frag");
        auto bloomCombineFragmentShaderSource = loadShaderSource("assets/shaders/bloomcombine.frag");
        auto blurFragmentShaderSource = loadShaderSource("assets/shaders/blur.frag");
        auto gammaCorrectionShaderSource = loadShaderSource("assets/shaders/gamma_correction.frag");
        auto cubeMapVertexShaderSource = loadShaderSource("assets/shaders/cube.vert");
        auto cubeMapFragmentShaderSource = loadShaderSource("assets/shaders/cube.frag");
        auto snowVertexShaderSource = loadShaderSource("assets/shaders/snow.vert");
        auto shadowVertexShaderSource = loadShaderSource("assets/shaders/shadow.vert");
        auto shadowGeometryShaderSource = loadShaderSource("assets/shaders/shadow.geom");
        auto depthVisualizationFragmentShaderSource = loadShaderSource("assets/shaders/visualize_depth_map.frag");

        // Create cube shader program

        GLuint cubeVertexShader = createShader(GL_VERTEX_SHADER, cubeVertexShaderSource);
        GLuint cubeFragmentShader = createShader(GL_FRAGMENT_SHADER, cubeFragmentShaderSource);
        cubeShaderProgram = createProgram({cubeVertexShader, cubeFragmentShader});
        GLuint snowVertexShader = createShader(GL_VERTEX_SHADER, snowVertexShaderSource);
        snowShaderProgram = createProgram({snowVertexShader, cubeFragmentShader});
        glDeleteShader(snowVertexShader);
        GLuint cubeNormalVertexShader = createShader(GL_VERTEX_SHADER, cubeNormalVertexShaderSource);
        GLuint cubeNormalGeometryShader = createShader(GL_GEOMETRY_SHADER, cubeNormalGeometryShaderSource);
        GLuint cubeNormalFragmentShader = createShader(GL_FRAGMENT_SHADER, cubeNormalFragmentShaderSource);
        cubeNormalShaderProgram = createProgram({cubeNormalVertexShader, cubeNormalGeometryShader, cubeNormalFragmentShader});
        glDeleteShader(cubeVertexShader);
        glDeleteShader(cubeFragmentShader);
        glDeleteShader(cubeNormalGeometryShader);
        glDeleteShader(cubeNormalFragmentShader);

        // Create lamp shader programs

        GLuint lampVertexShader = createShader(GL_VERTEX_SHADER, lampVertexShaderSource);
        GLuint lampFragmentShader = createShader(GL_FRAGMENT_SHADER, lampFragmentShaderSource);
        lampShaderProgram = createProgram({lampVertexShader, lampFragmentShader});
        glDeleteShader(lampFragmentShader);

        GLuint lampBorderFragmentShader = createShader(GL_FRAGMENT_SHADER, lampBorderFragmentShaderSource);
        lampBorderShaderProgram = createProgram({lampVertexShader, lampBorderFragmentShader});
        glDeleteShader(lampBorderFragmentShader);

        glDeleteShader(lampVertexShader);

        // Cube map shader program

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

        // Create gamma correction shader program

        GLuint gammaCorrectionFragmentShader = createShader(GL_FRAGMENT_SHADER, gammaCorrectionShaderSource);
        gammaCorrectionShaderProgram = createProgram({screenRectVertexShader, gammaCorrectionFragmentShader});
        glDeleteShader(gammaCorrectionFragmentShader);

        // Create depth visualization shader program

        GLuint depthVizualizationFragmentShader = createShader(GL_FRAGMENT_SHADER, depthVisualizationFragmentShaderSource);
        depthVisualizationProgram = createProgram({screenRectVertexShader, depthVizualizationFragmentShader});
        glDeleteShader(depthVizualizationFragmentShader);

        glDeleteShader(screenRectVertexShader);

        // Create shadow shader program
        GLuint shadowVertexShader = createShader(GL_VERTEX_SHADER, shadowVertexShaderSource);
        GLuint shadowGeometryShader = createShader(GL_GEOMETRY_SHADER, shadowGeometryShaderSource);
        shadowShaderProgram = createProgram({shadowVertexShader, shadowGeometryShader});
        glDeleteShader(shadowVertexShader);
    }

    auto [cubeVertexData, cubeVertexIndices, cubeMaterial] = loadModelData("assets/meshes/cube.obj")[0];
    auto [pyramidVertexData, pyramidVertexIndices, pyramidMaterial] = loadModelData("assets/meshes/pyramid.obj")[0];
    auto [circularPlaneVertexData, circularPlaneVertexIndices, circularPlaneMaterial] = loadModelData("assets/meshes/circularplane.obj")[0];
    auto [coneVertexData, coneVertexIndices, coneMaterial] = loadModelData("assets/meshes/cone.obj")[0];
    auto [sphereVertexData, sphereVertexIndices, sphereMaterial] = loadModelData("assets/meshes/sphere.obj")[0];
    auto [squarePlaneVertexData, squarePlaneVertexIndices, squarePlaneMaterial] = loadModelData("assets/meshes/squareplane.obj")[0];
    auto [skyboxVertexData, skyboxVertexIndices, skyboxMaterial] = loadModelData("assets/meshes/skybox.obj")[0];
    auto [transparentObjectVertexData, transparentObjectVertexIndices, transparentObjectMaterial] = loadModelData("assets/meshes/transparentplane.obj")[0];

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
    storeData(transparentObjectVertexData, transparentObjectVertexIndices, transparentVBO, transparentEBO);
    storeData(skyboxVertexData, skyboxVertexIndices, skyboxVBO, skyboxEBO);
    setupSnowData(snowPosVBO, snowDirVBO, numSnowParticles, 30.0, 30.0f, 20.0f, -1.0f);

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
    setupLamp(skyboxVAO, skyboxVBO, skyboxEBO);

    glBindVertexArray(snowVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, snowPosVBO);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, snowDirVBO);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // Set floor texture wrapping parameters

    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(circularPlaneMaterial.diffuseMap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId2D(circularPlaneMaterial.specularMap, false));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenBuffers(uniformBuffers.size(), uniformBuffers.data());

    glBindBuffer(GL_UNIFORM_BUFFER, matrixUBO);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matrixUBO);

    constexpr int LIGHT_BUFFER_SIZE = POINT_LIGHT_SIZE * MAX_POINT_LIGHTS +
                                      SPOT_LIGHT_SIZE * MAX_SPOT_LIGHTS +
                                      DIRECTIONAL_LIGHT_SIZE * MAX_DIRECTIONAL_LIGHTS +
                                      64 * (MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS + MAX_DIRECTIONAL_LIGHTS) +
                                      16;
    glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
    glBufferData(GL_UNIFORM_BUFFER, LIGHT_BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);
    std::vector<std::byte> bufferData(LIGHT_BUFFER_SIZE);
    auto bufferPointer = bufferData.data();
    for (std::size_t i = 0; i < pointLights.size(); i++) {
        int offset = POINT_LIGHT_SIZE * i;
        const auto& light = pointLights[i];
        copyLightColor(bufferPointer + offset, light);
        copyLightPosition(bufferPointer + offset + 48, light);
        copyLightRadius(bufferPointer + offset + 60, light);
    }
    for (std::size_t i = 0; i < spotLights.size(); i++) {
        int offset = POINT_LIGHT_SIZE * MAX_POINT_LIGHTS + SPOT_LIGHT_SIZE * i;
        const auto& light = spotLights[i];
        copyLightColor(bufferPointer + offset, light);
        copyLightPosition(bufferPointer + offset + 48, light);
        copyLightRadius(bufferPointer + offset + 60, light);
        copyLightDirection(bufferPointer + offset + 64, light);
        copyLightAngle(bufferPointer + offset + 76, light);
    }
    for (std::size_t i = 0; i < directionalLights.size(); i++) {
        int offset = POINT_LIGHT_SIZE * MAX_POINT_LIGHTS + SPOT_LIGHT_SIZE * MAX_SPOT_LIGHTS + DIRECTIONAL_LIGHT_SIZE * i;
        const auto& light = directionalLights[i];
        copyLightColor(bufferPointer + offset, light);
        copyLightDirection(bufferPointer + offset + 48, light);
    }
    {
        int baseOffset = LIGHT_BUFFER_SIZE - 16;
        std::memcpy(bufferPointer + baseOffset + 0, &numPointLights, 4);
        std::memcpy(bufferPointer + baseOffset + 8, &numDirectionalLights, 4);
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 0, LIGHT_BUFFER_SIZE, bufferData.data());
    bufferData.clear();
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, lightUBO);

    glUseProgram(cubeShaderProgram);
    glUniformBlockBinding(cubeShaderProgram, glGetUniformBlockIndex(cubeShaderProgram, "MatrixBlock"), 0);
    glUniformBlockBinding(cubeShaderProgram, glGetUniformBlockIndex(cubeShaderProgram, "LightsBlock"), 1);
    glUniform1i(glGetUniformLocation(cubeShaderProgram, "material.diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(cubeShaderProgram, "material.specularMap"), 1);
    glUniform1i(glGetUniformLocation(cubeShaderProgram, "pointLightShadowMapArray"), 10);
    glUniform1i(glGetUniformLocation(cubeShaderProgram, "spotLightShadowMapArray"), 11);
    glUniform1i(glGetUniformLocation(cubeShaderProgram, "dirLightShadowMapArray"), 12);

    glUseProgram(cubeNormalShaderProgram);
    glUniformBlockBinding(cubeNormalShaderProgram, glGetUniformBlockIndex(cubeNormalShaderProgram, "MatrixBlock"), 0);
    glUniform1f(glGetUniformLocation(cubeNormalShaderProgram, "normalScale"), 0.2f);
    glUniform3fv(glGetUniformLocation(cubeNormalShaderProgram, "normalColor"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.0f)));

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

    glUseProgram(snowShaderProgram);
    glUniformBlockBinding(snowShaderProgram, glGetUniformBlockIndex(snowShaderProgram, "MatrixBlock"), 0);
    glUniformBlockBinding(snowShaderProgram, glGetUniformBlockIndex(snowShaderProgram, "LightsBlock"), 1);
    glUniformMatrix4fv(glGetUniformLocation(snowShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(snowParticleModel));
    glUniform1f(glGetUniformLocation(snowShaderProgram, "freq"), 0.1f);
    glUniform1i(glGetUniformLocation(snowShaderProgram, "material.diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(snowShaderProgram, "material.specularMap"), 1);
    glUniform1i(glGetUniformLocation(snowShaderProgram, "pointLightShadowMapArray"), 10);
    glUniform1i(glGetUniformLocation(snowShaderProgram, "spotLightShadowMapArray"), 11);
    glUniform1i(glGetUniformLocation(snowShaderProgram, "dirLightShadowMapArray"), 12);
    glUniform1f(glGetUniformLocation(snowShaderProgram, "material.shininess"), 64.0f);

    glUseProgram(gammaCorrectionShaderProgram);
    glUniform1i(glGetUniformLocation(gammaCorrectionShaderProgram, "inputFrame"), 0);
    glUniform1f(glGetUniformLocation(gammaCorrectionShaderProgram, "correctionFactor"), 1.0f / gammaValue);

    glUseProgram(shadowShaderProgram);

    float forwardAxisValue, rightAxisValue, upAxisValue;

    float previousTime = 0.0f;

    auto window = CameraManager::getWindow();

    while (not glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - previousTime;

        forwardAxisValue = 0.0f;
        rightAxisValue = 0.0f;
        upAxisValue = 0.0f;
        bFlashLight = false;
        bGreyScale = false;

        // Input

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            bTAA = false;
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
            bTAA = true;
        }
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
            bMSAA = false;
        }
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
            bMSAA = true;
        }
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
            bBloom = false;
        }
        if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
            bBloom = true;
        }
        if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) {
            bSkybox = false;
        }
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
            bSkybox = true;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
            bBorder = false;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
            bBorder = true;
        }
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            bSnow = false;
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            bSnow = true;
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            bGammaCorrect = false;
        }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            bGammaCorrect = true;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            CameraManager::disableCameraLook();
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            CameraManager::enableCameraLook();
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            forwardAxisValue += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            forwardAxisValue -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            rightAxisValue += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            rightAxisValue -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            upAxisValue -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
            upAxisValue += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
            bGreyScale = true;
        }
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            bShowMag = true;
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            bShowMag = false;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            bFlashLight = true;
        }

        auto cameraForward = camera->getCameraForwardVector();
        auto cameraRight = camera->getCameraRightVector();
        auto cameraUp = camera->getCameraUpVector();
        cameraForward.z = 0.0f;
        cameraRight.z = 0.0f;
        cameraUp.x = cameraUp.y = 0.0f;
        glm::vec3 inputVector = cameraForward * forwardAxisValue + cameraRight * rightAxisValue + cameraUp * upAxisValue;
        if (glm::length(inputVector) > 0.0f) {
            camera->addLocationOffset(glm::normalize(inputVector) * deltaTime * cameraSpeed);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
        if (bFlashLight and numSpotLights > 0) {
            spotLights.back().direction = camera->getCameraForwardVector();
            spotLights.back().position = camera->getCameraPos();
            int offset = POINT_LIGHT_SIZE * MAX_POINT_LIGHTS + SPOT_LIGHT_SIZE * (spotLights.size() - 1);
            glBufferSubData(GL_UNIFORM_BUFFER, offset + 48, 12, glm::value_ptr(spotLights.back().position));
            glBufferSubData(GL_UNIFORM_BUFFER, offset + 64, 12, glm::value_ptr(spotLights.back().direction));
        }
        {
            int numUsedSpotlights = std::max(numSpotLights - !bFlashLight, 0);
            int offset = LIGHT_BUFFER_SIZE - 16 + 4;
            glBufferSubData(GL_UNIFORM_BUFFER, offset, 4, &numUsedSpotlights);
        }

        glm::mat4 view = CameraManager::getViewMatrix(),
                  projection = CameraManager::getProjectionMatrix();
        if (bTAA) {
            glm::vec3 sampleTrans = TAASamplesPositions[colorIndex];
            sampleTrans.x /= windowW;
            sampleTrans.y /= windowH;
            projection = glm::translate(glm::mat4(1.0f), sampleTrans) * projection;
        }

        // Generate shadowmaps

        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glViewport(0, 0, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
        std::vector<glm::mat4> pointLightTransformMatrices;
        std::vector<glm::mat4> pointLightRenderTransformMatrices;
        std::vector<glm::mat4> spotLightTransformMatrices;
        std::vector<glm::mat4> directionalLightTransformMatrices;
        const std::vector<std::pair<glm::vec3, glm::vec3>> lightVectors = {
            {{1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
            {{-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
            {{0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
            {{0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}}};
        for (const auto& pl : pointLights) {
            const auto& lightPos = pl.position;
            float lightRadius = pl.radius;
            glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), 1.0f, lightRadius, 100.0f);
            for (const auto& [lightDir, lightUp]: lightVectors) {
                glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, lightUp);
                glm::mat4 lightTransform = lightProjection * lightView;
                pointLightRenderTransformMatrices.push_back(lightTransform);
            }
            pointLightTransformMatrices.push_back(glm::translate(glm::mat4(1.0f), -lightPos));
        }
        if (numPointLights) {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointLightShadowCubeMapArray, 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            drawShadowCasters(shadowShaderProgram, pointLightRenderTransformMatrices, cubeVAO, cubeMatrices, cubeVertexIndices.size(), pyramidVAO, pyramidMatrices, pyramidVertexIndices.size());
        }

        for (auto it = spotLights.begin(); it < spotLights.end() - !bFlashLight; it++) {
            const auto& sl = *it;
            const auto& lightDir = sl.direction;
            const auto& lightPos = sl.position;
            float lightCone = sl.outerAngleCos;
            float lightRadius = sl.radius;
            glm::vec3 lightUp = calculateLightUp(lightDir);
            glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, lightUp);
            glm::mat4 lightProjection = glm::perspective(2.0f * glm::acos(lightCone), 1.0f, lightRadius, 100.0f);
            glm::mat4 lightTransform = lightProjection * lightView;
            spotLightTransformMatrices.push_back(lightTransform);
        }
        int numUsedSpotLights = std::max(numSpotLights - !bFlashLight, 0);
        if (numUsedSpotLights) {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, spotLightShadowMapArray, 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            drawShadowCasters(shadowShaderProgram, spotLightTransformMatrices, cubeVAO, cubeMatrices, cubeVertexIndices.size(), pyramidVAO, pyramidMatrices, pyramidVertexIndices.size());
        }

        glViewport(0, 0, DIR_LIGHT_SHADOWMAP_RESOLUTION, DIR_LIGHT_SHADOWMAP_RESOLUTION);
        for (const auto& dl : directionalLights) {
            const auto& lightDir = dl.direction;
            glm::vec3 lightUp = calculateLightUp(lightDir);
            glm::mat4 lightView = glm::lookAt(camera->getCameraPos(), camera->getCameraPos() + lightDir, lightUp);
            float verticalFOV = CameraManager::getVerticalFOV();
            float horizontalFOV = CameraManager::getHorizontalFOV();
            glm::vec3 cameraLookPoint = camera->getCameraPos() + camera->getCameraForwardVector() * CameraManager::getFarPlane();
            glm::vec3 upVectorScaled = glm::tan(glm::radians(verticalFOV / 2.0f)) * CameraManager::getFarPlane() * camera->getCameraUpVector();
            glm::vec3 rightVectorScaled = glm::tan(glm::radians(horizontalFOV / 2.0f)) * CameraManager::getFarPlane() * camera->getCameraRightVector();
            std::vector<glm::vec3> cameraFrustrumPoints =
                {cameraLookPoint + upVectorScaled + rightVectorScaled,
                 cameraLookPoint + upVectorScaled - rightVectorScaled,
                 cameraLookPoint - upVectorScaled + rightVectorScaled,
                 cameraLookPoint - upVectorScaled - rightVectorScaled};
            std::for_each(cameraFrustrumPoints.begin(), cameraFrustrumPoints.end(), [&lightView](auto& e) { e = lightView * glm::vec4(e, 1.0f); });
            glm::vec3 minCoords(0.0f);
            glm::vec3 maxCoords(0.0f);
            minCoords = glm::min(minCoords, glm::min(cameraFrustrumPoints[0], glm::min(cameraFrustrumPoints[1], glm::min(cameraFrustrumPoints[2], cameraFrustrumPoints[3]))));
            maxCoords = glm::max(maxCoords, glm::max(cameraFrustrumPoints[0], glm::max(cameraFrustrumPoints[1], glm::max(cameraFrustrumPoints[2], cameraFrustrumPoints[3]))));
            maxCoords.z += CameraManager::getFarPlane();
            glm::mat4 lightProjection = glm::ortho(minCoords.x, maxCoords.x, minCoords.y, maxCoords.y, -maxCoords.z, -minCoords.z);
            glm::mat4 lightTransform = lightProjection * lightView;
            directionalLightTransformMatrices.push_back(lightTransform);
        }
        if (numDirectionalLights) {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, directionalLightShadowMapArray, 0);
            glClear(GL_DEPTH_BUFFER_BIT);
            drawShadowCasters(shadowShaderProgram, directionalLightTransformMatrices, cubeVAO, cubeMatrices, cubeVertexIndices.size(), pyramidVAO, pyramidMatrices, pyramidVertexIndices.size());
        }

        glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
        int offset = LIGHT_BUFFER_SIZE - 16 - 64 * MAX_DIRECTIONAL_LIGHTS;
        glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::mat4) * directionalLightTransformMatrices.size(), directionalLightTransformMatrices.data());
        offset -= 64 * MAX_SPOT_LIGHTS;
        glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::mat4) * spotLightTransformMatrices.size(), spotLightTransformMatrices.data());
        offset -= 64 * MAX_POINT_LIGHTS;
        glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(glm::mat4) * pointLightTransformMatrices.size(), pointLightTransformMatrices.data());
        glViewport(0, 0, windowW, windowH);

        // Start drawing

        glBindFramebuffer(GL_FRAMEBUFFER, blitFBO);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + colorIndex);
        if (bMSAA) {
            glBindFramebuffer(GL_FRAMEBUFFER, MSFBO);
        }
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glBindBuffer(GL_UNIFORM_BUFFER, matrixUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));

        // Setup model draw parameters

        glEnable(GL_CULL_FACE);

        glUseProgram(cubeShaderProgram);

        glUniform3fv(glGetUniformLocation(cubeShaderProgram, "cameraPos"), 1, glm::value_ptr(camera->getCameraPos()));
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, pointLightShadowCubeMapArray);
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D_ARRAY, spotLightShadowMapArray);
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_2D_ARRAY, directionalLightShadowMapArray);

        // Draw cubes

        setShaderMatrial(cubeShaderProgram, cubeMaterial);

        for (const auto& [m, n]: cubeMatrices) {
            setModelUniforms(cubeShaderProgram, m, n);
            glBindVertexArray(cubeVAO);
            glDrawElements(GL_TRIANGLES, cubeVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        // Draw pyramids

        setShaderMatrial(cubeShaderProgram, pyramidMaterial);

        for (const auto& [m, n]: pyramidMatrices) {
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

        if (bBorder) {
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
        }

        glUseProgram(lampShaderProgram);

        for (int i = 0; i < numPointLights; i++) {
            setLampUniforms(lampShaderProgram, pointLightMatrices[i], pointLights[i].diffuse);
            glBindVertexArray(pointLightVAO);
            glDrawElements(GL_TRIANGLES, sphereVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        for (int i = 0; i < numSpotLights - 1; i++) {
            setLampUniforms(lampShaderProgram, spotLightMatrices[i], spotLights[i].diffuse);
            glBindVertexArray(spotLightVAO);
            glDrawElements(GL_TRIANGLES, coneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glDisable(GL_CULL_FACE);

        for (int i = 0; i < numDirectionalLights; i++) {
            setLampUniforms(lampShaderProgram, directionalLightMatrices[i], directionalLights[i].diffuse);
            glBindVertexArray(directionalLightVAO);
            glDrawElements(GL_TRIANGLES, squarePlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glEnable(GL_CULL_FACE);

        // Draw lamp borders

        if (bBorder) {
            glStencilFunc(GL_GREATER, 1, 0xFF);
            glStencilMask(0x00);

            glm::mat4 silhoutte = glm::scale(glm::mat4(1.0f), 1.05f * glm::vec3(1.0f));
            glUseProgram(lampBorderShaderProgram);

            for (int i = 0; i < numPointLights; i++) {
                setLampUniforms(lampBorderShaderProgram, pointLightMatrices[i] * silhoutte, pointLights[i].diffuse);
                glBindVertexArray(pointLightVAO);
                glDrawElements(GL_TRIANGLES, sphereVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            }

            for (int i = 0; i < numSpotLights - 1; i++) {
                setLampUniforms(lampBorderShaderProgram, spotLightMatrices[i] * silhoutte, spotLights[i].diffuse);
                glBindVertexArray(spotLightVAO);
                glDrawElements(GL_TRIANGLES, coneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            }

            glDisable(GL_CULL_FACE);

            for (int i = 0; i < numDirectionalLights; i++) {
                setLampUniforms(lampBorderShaderProgram, directionalLightMatrices[i] * silhoutte, directionalLights[i].diffuse);
                glBindVertexArray(directionalLightVAO);
                glDrawElements(GL_TRIANGLES, squarePlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            }

            glEnable(GL_CULL_FACE);

            glDisable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilMask(0xFF);
        }

        // Draw snow

        if (bSnow) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, snowDiffuseTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, snowSpecularTexture);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, pointLightShadowCubeMapArray);
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D_ARRAY, spotLightShadowMapArray);
            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D_ARRAY, directionalLightShadowMapArray);
            glUseProgram(snowShaderProgram);
            glUniform1f(glGetUniformLocation(snowShaderProgram, "time"), glfwGetTime());
            glUniform3fv(glGetUniformLocation(snowShaderProgram, "cameraPos"), 1, glm::value_ptr(camera->getCameraPos()));
            glBindVertexArray(snowVAO);
            glDrawElementsInstanced(GL_TRIANGLES, sphereVertexIndices.size(), GL_UNSIGNED_INT, nullptr, numSnowParticles);
        }

        // Draw skybox

        if (bSkybox) {
            glCullFace(GL_FRONT);
            view = glm::mat4(glm::mat3(view));
            glUseProgram(cubeMapShaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(cubeMapShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(cubeMapShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, TextureLoader::getTextureIdCubeMap(cubeMapFaceTextures));
            glBindVertexArray(skyboxVAO);
            glDrawElements(GL_TRIANGLES, skyboxVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            glCullFace(GL_BACK);
        }

        // Draw transparent objects

        glEnable(GL_BLEND);

        glUseProgram(cubeShaderProgram);

        std::sort(transparentObjects.begin(), transparentObjects.end(),
                  [&camera](const std::tuple<glm::mat4, glm::mat3, Material>& rhs, const std::tuple<glm::mat4, glm::mat3, Material>& lhs) {
                      auto cameraPos = camera->getCameraPos();
                      auto rhs_pos = glm::vec3(std::get<0>(rhs)[3]);
                      auto lhs_pos = glm::vec3(std::get<0>(lhs)[3]);
                      return glm::length(rhs_pos - cameraPos) > glm::length(lhs_pos - cameraPos);
                  });

        for (const auto& [model, normal, material]: transparentObjects) {
            setShaderMatrial(cubeShaderProgram, material);
            setModelUniforms(cubeShaderProgram, model, normal);
            glBindVertexArray(transparentVAO);
            glDrawElements(GL_TRIANGLES, transparentObjectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glDisable(GL_BLEND);

        // Blit MSAA framebuffer

        if (bMSAA) {
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

        if (bTAA) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, frameTextureArray);
            glUseProgram(TAAShaderProgram);
            glBindVertexArray(screenRectVAO);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        } else {
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

            for (auto horizontal: {0, 1}) {
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

        if (bGreyScale) {
            glUseProgram(greyscaleShaderProgram);
            glBindTexture(GL_TEXTURE_2D, fullResPPTextures[fullResReadIndex]);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            swapBuffers(fullResReadIndex);
        }

        if (bGammaCorrect) {
            glUseProgram(gammaCorrectionShaderProgram);
            glBindTexture(GL_TEXTURE_2D, fullResPPTextures[fullResReadIndex]);
            glDrawElements(GL_TRIANGLES, rectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
            swapBuffers(fullResReadIndex);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, windowW, windowH, 0, 0, windowW, windowH, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        if (bShowMag) {
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
