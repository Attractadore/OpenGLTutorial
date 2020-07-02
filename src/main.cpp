#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "Lights.hpp"
#include "TextureLoader.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <IL/il.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <fstream>

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
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId(material.diffuseMap));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TextureLoader::getTextureId(material.specularMap));
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

int main(){
    std::string windowTitle = "OpenGL Tutorial";
    float horizontalFOV = 90.0f;
    float aspectRatio = 16.0f / 9.0f;
    int defaultWindowHeight = 720;
    int defaultWindowWidth = defaultWindowHeight * aspectRatio;
    float cameraSpeed = 5.0f;

    constexpr int MAX_POINT_LIGHTS = 10;
    constexpr int MAX_SPOT_LIGHTS = 10;
    constexpr int MAX_DIRECTIONAL_LIGHTS = 10;

    int numPointLights = 5;
    int numSpotLights = 5 + 1;
    int numDirectionalLights = 1;

    numPointLights = std::min(numPointLights, MAX_POINT_LIGHTS);
    numSpotLights = std::min(numSpotLights, MAX_SPOT_LIGHTS);
    numDirectionalLights = std::min(numDirectionalLights, MAX_DIRECTIONAL_LIGHTS);

    std::vector<PointLight> pointLights(numPointLights);
    std::vector<SpotLight> spotLights(numSpotLights);
    std::vector<DirectionalLight> directionalLights(numDirectionalLights);

    bool bFlashLight = false;
    bool bGreyScale = false;
    bool bTAA = true;
    int TAASamples = 4;
    std::vector<glm::vec3> TAASamplesPositions = {
        {-0.25f, -0.25f, 0.0f},
        { 0.25f, -0.25f, 0.0f},
        { 0.25f,  0.25f, 0.0f},
        {-0.25f,  0.25f, 0.0f},
    };
    bool bMSAA = true;
    int MSAASamples = 4;
    bool bShowMag = false;

    int numCubesX = 10;
    int numCubesY = numCubesX;
    float cubeDistanceX = 4.0f;
    float cubeDistanceY = cubeDistanceX;

    std::vector<std::pair<glm::mat4, glm::mat3>> pyramidMatrices;
    std::vector<std::pair<glm::mat4, glm::mat3>> cubeMatrices;
    std::vector<glm::mat4> pointLightMatrices;
    std::vector<glm::mat4> spotLightMatrices;
    std::vector<glm::mat4> directionalLightMatrices;

    glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -1.0f});
    glm::mat3 floorNormal(1.0f);

    calculatePyramidMatrices(numCubesX, numCubesY, cubeDistanceX, cubeDistanceY, pyramidMatrices);
    calculateCubeMatrices(numCubesX, numCubesY, cubeDistanceX, cubeDistanceY, cubeMatrices);
    calculatePointLightMatrices(pointLights, pointLightMatrices);
    calculateSpotLightMatrices(spotLights, spotLightMatrices);
    calculateDirectionalLightMatrices(directionalLights, directionalLightMatrices);

    glm::vec3 cameraStartPos = {5.0f, 5.0f, 3.0f};
    glm::vec3 cameraStartLookDirection = -cameraStartPos;
    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDirection, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 view;
    glm::mat4 projection;

    GLuint cubeShaderProgram, lampShaderProgram, TAAShaderProgram, postprocessShaderProgram;
    GLuint frameTextureArray;
    GLuint combinedFrameTexture;
    int colorIndex = 0;

    std::vector<GLuint> vertexBuffers(8);

    GLuint& cubeVBO = vertexBuffers[0];
    GLuint& pyramidVBO = vertexBuffers[1];
    GLuint& circlePlaneVBO = vertexBuffers[2];
    GLuint& sphereVBO = vertexBuffers[3];
    GLuint& coneVBO = vertexBuffers[4];
    GLuint& squarePlaneVBO = vertexBuffers[5];
    GLuint& screenRectVBO = vertexBuffers[6];
    GLuint& magRectVBO = vertexBuffers[7];

    std::vector<GLuint> elementBuffers(7);

    GLuint& cubeEBO = elementBuffers[0];
    GLuint& pyramidEBO = elementBuffers[1];
    GLuint& planeEBO = elementBuffers[2];
    GLuint& sphereEBO = elementBuffers[3];
    GLuint& coneEBO = elementBuffers[4];
    GLuint& squarePlaneEBO = elementBuffers[5];
    GLuint& screenRectEBO = elementBuffers[6];

    std::vector<GLuint> uniformBuffers(2);
    GLuint& matrixUBO = uniformBuffers[0];
    GLuint& lightUBO = uniformBuffers[1];

    std::vector<GLuint> frameBuffers(3);
    GLuint& TAAFBO = frameBuffers[0];
    GLuint& MSFBO = frameBuffers[1];
    GLuint& blitFBO = frameBuffers[2];

    std::vector<GLuint> renderBuffers(3);
    GLuint& MSColorRenderBuffer = renderBuffers[0];
    GLuint& MSDepthStencilRenderBuffer = renderBuffers[1];
    GLuint& blitDepthStencilRenderBuffer = renderBuffers[2];

    std::vector<GLuint> vertexArrays(8);

    GLuint& cubeVAO = vertexArrays[0];
    GLuint& pyramidVAO = vertexArrays[1];
    GLuint& floorVAO = vertexArrays[2];
    GLuint& pointLightVAO = vertexArrays[3];
    GLuint& spotLightVAO = vertexArrays[4];
    GLuint& directionalLightVAO = vertexArrays[5];
    GLuint& screenRectVAO = vertexArrays[6];
    GLuint& magRectVAO = vertexArrays[7];

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

    constexpr std::array<GLuint, 6> screenRectVertexIndices = {
        0, 1, 2,
        2, 3, 0
    };

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
    CameraManager::setViewportSize(defaultWindowWidth, defaultWindowHeight);
    CameraManager::setHorizontalFOV(horizontalFOV);
    CameraManager::setActiveCamera(camera);
    enableCameraLook(window);

    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    if (glIsEnabled(GL_DEBUG_OUTPUT)){
        glDebugMessageCallback(debugFunction, nullptr);
    }
    glEnable(GL_DEPTH_TEST);

    glGenFramebuffers(frameBuffers.size(), frameBuffers.data());
    glGenRenderbuffers(renderBuffers.size(), renderBuffers.data());

    // Setup rendering textures

    glGenTextures(1, &combinedFrameTexture);
    glBindTexture(GL_TEXTURE_2D, combinedFrameTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, defaultWindowWidth, defaultWindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &frameTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, frameTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, defaultWindowWidth, defaultWindowHeight, TAASamples, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Setup renderbuffers

    glBindRenderbuffer(GL_RENDERBUFFER, MSColorRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAASamples, GL_RGB, defaultWindowWidth, defaultWindowHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, MSDepthStencilRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAASamples, GL_DEPTH24_STENCIL8, defaultWindowWidth, defaultWindowHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, blitDepthStencilRenderBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH24_STENCIL8, defaultWindowWidth, defaultWindowHeight);

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

    // Setup TAA framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, TAAFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, combinedFrameTexture, 0);

    CameraManager::framebuffers = frameBuffers;

    {
      auto cubeVertexShaderSource = loadShaderSource("assets/shaders/triangle.vert");
      auto cubeFragmentShaderSource = loadShaderSource("assets/shaders/triangle.frag");
      auto lampVertexShaderSource = loadShaderSource("assets/shaders/lamp.vert");
      auto lampFragmentShaderSource = loadShaderSource("assets/shaders/lamp.frag");
      auto screenRectVertexShaderSource = loadShaderSource("assets/shaders/screenrect.vert");
      auto TAAFragmentShaderSource = loadShaderSource("assets/shaders/taa.frag");
      auto postprocessFragmentShaderSource = loadShaderSource("assets/shaders/postprocess.frag");

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

      GLuint screenRectVertexShader = createShader(GL_VERTEX_SHADER, screenRectVertexShaderSource);

      // Create taa shader program

      GLuint TAAFragmentShader = createShader(GL_FRAGMENT_SHADER, TAAFragmentShaderSource);
      TAAShaderProgram = createProgram({screenRectVertexShader, TAAFragmentShader});
      glDeleteShader(TAAFragmentShader);

      // Create greyscale shader program

      GLuint postprocessFragmentShader = createShader(GL_FRAGMENT_SHADER, postprocessFragmentShaderSource);
      postprocessShaderProgram = createProgram({screenRectVertexShader, postprocessFragmentShader});
      glDeleteShader(postprocessFragmentShader);

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
    storeData(screenRectVertexData, screenRectVertexIndices, screenRectVBO, screenRectEBO);
    storeData(magRectVertexData, screenRectVertexIndices, magRectVBO, 0);

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

    float forwardAxisValue, rightAxisValue, upAxisValue;

    float previousTime = 0.0f;

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

        auto [screenW, screenH] = CameraManager::getViewportSize();

        view = CameraManager::getViewMatrix();
        projection = CameraManager::getProjectionMatrix();
        if (bTAA){
            glm::vec3 sampleTrans = TAASamplesPositions[colorIndex];
            sampleTrans.x /= screenW;
            sampleTrans.y /= screenH;
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

        setShaderMatrial(cubeShaderProgram, circularPlaneMaterial);

        setModelUniforms(cubeShaderProgram, floorModel, floorNormal);
        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, circularPlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);

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

        for (int i = 0; i < numDirectionalLights; i++){
            setLampUniforms(lampShaderProgram, directionalLightMatrices[i], directionalLights[i].diffuse);
            glBindVertexArray(directionalLightVAO);
            glDrawElements(GL_TRIANGLES, squarePlaneVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        if (bMSAA){
            glBindFramebuffer(GL_READ_FRAMEBUFFER, MSFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blitFBO);
            glBlitFramebuffer(0, 0, screenW, screenH, 0, 0, screenW, screenH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }

        glDisable(GL_DEPTH_TEST);

        if (bTAA){
            glBindFramebuffer(GL_FRAMEBUFFER, TAAFBO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, frameTextureArray);
            glUseProgram(TAAShaderProgram);
            glBindVertexArray(screenRectVAO);
            glDrawElements(GL_TRIANGLES, screenRectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }
        else {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, blitFBO);
            glReadBuffer(GL_COLOR_ATTACHMENT0 + colorIndex);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, TAAFBO);
            glBlitFramebuffer(0, 0, screenW, screenH, 0, 0, screenW, screenH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(postprocessShaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, combinedFrameTexture);
        glUniform1i(glGetUniformLocation(postprocessShaderProgram, "bGreyScale"), bGreyScale);
        glBindVertexArray(screenRectVAO);
        glDrawElements(GL_TRIANGLES, screenRectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        if (bShowMag){
            glBindVertexArray(magRectVAO);
            glDrawElements(GL_TRIANGLES, screenRectVertexIndices.size(), GL_UNSIGNED_INT, nullptr);
        }

        glEnable(GL_DEPTH_TEST);

        colorIndex = (colorIndex + 1) % TAASamples;

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
