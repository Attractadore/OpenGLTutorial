#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "debug.hpp"
#include "load_model.hpp"
#include "load_shader.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::vec3 getLightDir(glm::vec3 const& lightDir) {
    return glm::normalize(lightDir);
}

glm::vec3 getLightUp(glm::vec3 const& lightDir) {
    glm::vec3 lightUp{0.0f, 0.0f, 1.0f};
    glm::vec3 lightRight = glm::cross(lightDir, lightUp);
    lightUp = glm::cross(lightRight, lightDir);
    return glm::normalize(lightUp);
}

glm::mat4 getLightView(glm::vec3 const& lightDir, glm::vec3 const& lightUp) {
    return glm::lookAt(glm::vec3{0.0f}, lightDir, lightUp);
}

glm::vec3 projToWorld(glm::mat4 const& projViewInv, glm::vec3 const& frustP) {
    glm::vec4 worldP = projViewInv * glm::vec4{frustP, 1.0f};
    return worldP / worldP.w;
}

float getFrustrumDiag(glm::mat4 const& projViewInv) {
    glm::vec3 v0 = projToWorld(projViewInv, {-1.0f, -1.0f, 1.0f});
    glm::vec3 v1 = projToWorld(projViewInv, {1.0f, 1.0f, -1.0f});
    glm::vec3 v2 = projToWorld(projViewInv, {1.0f, 1.0f, 1.0f});

    float r1 = glm::length(v1 - v0);
    float r2 = glm::length(v2 - v0);

    return glm::max(r1, r2);
}

float discretize(float v, float step) {
    return glm::floor(v / step) * step;
}

glm::mat4 getLightProj(glm::mat4 const& lightView, glm::mat4 const& cameraProjViewInv, float boxSize, float shadowSampleSize) {
    glm::vec3 maxBox{std::numeric_limits<float>::lowest()};
    glm::vec3 minBox{std::numeric_limits<float>::max()};
    for (float x: {-1.0f, 1.0f}) {
        for (float y: {-1.0f, 1.0f}) {
            for (float z: {-1.0f, 1.0f}) {
                glm::vec3 frustP{x, y, z};
                glm::vec3 worldP = projToWorld(cameraProjViewInv, frustP);
                glm::vec3 viewP = lightView * glm::vec4{worldP, 1.0f};
                viewP.z *= -1.0f;
                maxBox = glm::max(maxBox, viewP);
                minBox = glm::min(minBox, viewP);
            }
        }
    }

    minBox.x = discretize(minBox.x, shadowSampleSize);
    minBox.y = discretize(minBox.y, shadowSampleSize);
    maxBox.x = minBox.x + boxSize;
    maxBox.y = minBox.y + boxSize;

    glm::mat4 lightProj = glm::ortho(minBox.x, maxBox.x, minBox.y, maxBox.y, minBox.z, maxBox.z);

    return lightProj;
}

std::vector<float> Partition(float nearPlane, float farPlane, std::uint8_t numCascades) {
    std::vector<float> cascadeBounds;
    const float cascadeScale = std::pow(farPlane / nearPlane, 1.0f / numCascades);
    float cascadeNearPlane = nearPlane;
    for (std::uint8_t i = 0; i < numCascades; i++) {
        cascadeBounds.push_back(cascadeNearPlane);
        cascadeNearPlane *= cascadeScale;
    }
    cascadeBounds.push_back(farPlane);
    return cascadeBounds;
}

float ConvertDepth(float nearPlane, float farPlane, float z) {
    return (z - nearPlane) / (farPlane - nearPlane) * farPlane / z;
}

struct CascadeProperties {
    std::vector<glm::mat4> transforms;
    glm::vec4 sampleSizes, depths;
};

CascadeProperties GetCascadeProperties(float nearPlane, float farPlane, std::uint8_t numCascades, std::uint16_t shadowRes, glm::mat4 const& m4View, float verticalFOV, float aspectRatio, glm::mat4 const& lightView) {
    CascadeProperties properties;
    const auto cascadeBounds = Partition(nearPlane, farPlane, numCascades);
    for (std::size_t i = 0; i < numCascades; i++) {
        const float n = cascadeBounds[i];
        const float f = cascadeBounds[i + 1];
        const glm::mat4 cascadeProj = glm::perspective(glm::radians(verticalFOV), aspectRatio, n, f);
        const glm::mat4 cascadeProjViewInv = glm::inverse(cascadeProj * m4View);
        // Cascade bounding box must be larger than the frustrum
        float boundingBoxSize = getFrustrumDiag(cascadeProjViewInv) * (shadowRes + 1) / shadowRes;
        float sampleSize = boundingBoxSize / shadowRes;
        const glm::mat4 lightProj = getLightProj(lightView, cascadeProjViewInv, boundingBoxSize, sampleSize);
        properties.transforms.push_back(lightProj * lightView);
        properties.sampleSizes[i] = sampleSize;
        const float nDepth = ConvertDepth(nearPlane, farPlane, n);
        properties.depths[i] = nDepth;
    }
    return properties;
}

template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
T const* DataPtr(std::vector<glm::mat<C, R, T, Q>> const& data) {
    return reinterpret_cast<T const*>(data.data());
}

std::size_t Ceil(std::size_t x, std::size_t y) {
    return x / y + (x % y != 0);
}

int main() {
    const std::string assetsPath = "assets";
    const std::string meshesPath = assetsPath + "/meshes/shadows";
    const std::string texturePath = assetsPath + "/textures/shadows";
    const std::string shaderBinPath = assetsPath + "/shaders/bin/shadows";

    std::size_t viewportW = 1280;
    std::size_t viewportH = 720;

    double cameraSpeed = 5.0;
    glm::vec3 cameraStartPos = {0.0f, -10.0f, 4.0f};
    glm::vec3 cameraStartLookDir = -cameraStartPos;
    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDir, glm::vec3{0.0f, 0.0f, 1.0f});

    CameraManager::initialize(viewportW, viewportH);
    CameraManager::currentCamera = camera;
    CameraManager::enableCameraLook();
    CameraManager::setFarPlane(20.0f);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    activateGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);

    MeshGLRepr bunnyMesh = createMeshGLRepr(meshesPath + "/bunny.obj");
    glm::mat4 bunnyModel{1.0f};
    glm::mat3 bunnyNormal{1.0f};
    MeshGLRepr groundMesh = createMeshGLRepr(meshesPath + "/../circularplane.obj");
    glm::mat4 groundModel = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f});
    glm::mat3 groundNormal{1.0f};

    GLuint lightingShaderProgram;
    {
        GLuint lightingVertexShader = createShaderSPIRV(GL_VERTEX_SHADER, shaderBinPath + "/lighting.vert.spv");
        GLuint lightingFragmentShader = createShaderSPIRV(GL_FRAGMENT_SHADER, shaderBinPath + "/lighting.frag.spv");
        lightingShaderProgram = createProgram({lightingVertexShader, lightingFragmentShader});
        glDeleteShader(lightingVertexShader);
        glDeleteShader(lightingFragmentShader);
    }
    GLuint shadowShaderProgram = 0;
    GLuint zPrepassShaderProgram = 0;
    {
        GLuint shadowVertexShader = createShaderSPIRV(GL_VERTEX_SHADER, shaderBinPath + "/shadow.vert.spv");
        GLuint shadowGeomShader = createShaderSPIRV(GL_GEOMETRY_SHADER, shaderBinPath + "/shadow.geom.spv");
        GLuint zPrepassFragmentShader = createShaderSPIRV(GL_FRAGMENT_SHADER, shaderBinPath + "/ZPrepass.frag.spv");
        shadowShaderProgram = createProgram({shadowVertexShader, shadowGeomShader});
        zPrepassShaderProgram = createProgram({shadowVertexShader, zPrepassFragmentShader});
        glDeleteShader(shadowVertexShader);
        glDeleteShader(shadowGeomShader);
        glDeleteShader(zPrepassFragmentShader);
    }
    GLuint reduceShaderProgram = 0;
    {
        GLuint minShader = createShaderSPIRV(GL_COMPUTE_SHADER, shaderBinPath + "/Reduce.comp.spv");
        reduceShaderProgram = createProgram({minShader});
        glDeleteShader(minShader);
    }
    glm::ivec3 workGroupSize;
    glGetProgramiv(reduceShaderProgram, GL_COMPUTE_WORK_GROUP_SIZE, glm::value_ptr(workGroupSize));

    glm::vec3 lightDir = getLightDir({1.0f, 0.0f, -1.0f});
    glm::vec3 lightUp = getLightUp(lightDir);
    glm::mat4 lightView = getLightView(lightDir, lightUp);

    GLuint shadowSampler;
    glCreateSamplers(1, &shadowSampler);
    glSamplerParameteri(shadowSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(shadowSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(shadowSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(shadowSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glSamplerParameterfv(shadowSampler, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(glm::vec4{1.0f}));
    glSamplerParameteri(shadowSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(shadowSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    uint16_t shadowMapRes = 1024;
    uint8_t shadowMapNumCascades = 4;
    GLuint shadowMapArray;
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &shadowMapArray);
    glTextureStorage3D(shadowMapArray, 1, GL_DEPTH_COMPONENT32, shadowMapRes, shadowMapRes, shadowMapNumCascades);

    GLuint shadowFramebuffer = 0;
    glCreateFramebuffers(1, &shadowFramebuffer);
    glNamedFramebufferTexture(shadowFramebuffer, GL_DEPTH_ATTACHMENT, shadowMapArray, 0);
    glNamedFramebufferDrawBuffer(shadowFramebuffer, GL_NONE);
    glNamedFramebufferReadBuffer(shadowFramebuffer, GL_NONE);

    GLuint drawColorRenderbuffer = 0;
    glCreateRenderbuffers(1, &drawColorRenderbuffer);
    glNamedRenderbufferStorage(drawColorRenderbuffer, GL_RGBA8, viewportW, viewportH);
    GLuint drawDepthRenderbuffer = 0;
    glCreateRenderbuffers(1, &drawDepthRenderbuffer);
    glNamedRenderbufferStorage(drawDepthRenderbuffer, GL_DEPTH_COMPONENT32, viewportW, viewportH);

    // Working with depth textures in compute shaders is not supported.
    // Use separate texture array and write depth to it as color during a z only prepass.
    std::size_t queueSize = 3;
    GLuint drawDepthTextureArray = 0;
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &drawDepthTextureArray);
    GLenum drawDepthInternalFormat = GL_R8;
    GLenum drawDepthFormat = GL_RED;
    glTextureStorage3D(drawDepthTextureArray, 1, drawDepthInternalFormat, viewportW, viewportH, queueSize);

    std::vector<GLuint> writeDepthBuffers(queueSize);
    glCreateBuffers(writeDepthBuffers.size(), writeDepthBuffers.data());
    for (const auto& writeDepthBuffer: writeDepthBuffers) {
        glNamedBufferStorage(writeDepthBuffer, sizeof(GLuint), nullptr, 0);
    }

    std::vector<GLuint> readDepthBuffers(queueSize);
    glCreateBuffers(readDepthBuffers.size(), readDepthBuffers.data());
    for (const auto& readDepthBuffer: readDepthBuffers) {
        // GL_CLIENT_STORAGE_BIT alone is not enough to get cpu side storage
        glNamedBufferStorage(readDepthBuffer, sizeof(GLuint), nullptr, GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT);
        glClearNamedBufferData(readDepthBuffer, drawDepthInternalFormat, drawDepthFormat, GL_UNSIGNED_INT, nullptr);
    }

    std::size_t drawI = 0;

    GLuint drawFramebuffer;
    glCreateFramebuffers(1, &drawFramebuffer);
    glNamedFramebufferRenderbuffer(drawFramebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, drawColorRenderbuffer);
    glNamedFramebufferRenderbuffer(drawFramebuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, drawDepthRenderbuffer);

    double currentTime = 0;
    double previousTime = 0;
    auto window = CameraManager::getWindow();
    while (!glfwWindowShouldClose(window)) {
        previousTime = currentTime;
        currentTime = glfwGetTime();
        double deltaTime = currentTime - previousTime;

        CameraManager::processEvents();

        glm::vec3 cameraMovementInput = CameraManager::getCameraMovementInput();
        if (glm::length(cameraMovementInput) > 0.0f) {
            cameraMovementInput = glm::normalize(cameraMovementInput);
            camera->cameraPos += float(deltaTime * cameraSpeed) * cameraMovementInput;
        }

        // Get min depth

        GLuint minDepth = 0;
        glGetNamedBufferSubData(readDepthBuffers[drawI], 0, sizeof(minDepth), &minDepth);
        {
            constexpr GLuint clearDepth = -1U;
            glClearNamedBufferData(writeDepthBuffers[drawI], drawDepthInternalFormat, drawDepthFormat, GL_UNSIGNED_INT, &clearDepth);
        }

        glm::mat4 projView = CameraManager::getProjectionMatrix() * CameraManager::getViewMatrix();

        // Do z prepass

        glBindFramebuffer(GL_FRAMEBUFFER, drawFramebuffer);
        glNamedFramebufferTextureLayer(drawFramebuffer, GL_COLOR_ATTACHMENT1, drawDepthTextureArray, 0, drawI);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(zPrepassShaderProgram);

        glProgramUniformMatrix4fv(zPrepassShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projView * bunnyModel));
        glBindVertexArray(bunnyMesh.VAO);
        glDrawElements(GL_TRIANGLES, bunnyMesh.numIndices, GL_UNSIGNED_INT, nullptr);

        glProgramUniformMatrix4fv(zPrepassShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projView * groundModel));
        glBindVertexArray(groundMesh.VAO);
        glDrawElements(GL_TRIANGLES, groundMesh.numIndices, GL_UNSIGNED_INT, nullptr);

        glNamedFramebufferTextureLayer(drawFramebuffer, GL_COLOR_ATTACHMENT1, 0, 0, 0);

        // Dispatch compute for min depth

        glUseProgram(reduceShaderProgram);
        glProgramUniform2ui(reduceShaderProgram, 0, viewportW, viewportH);
        glBindImageTexture(0, drawDepthTextureArray, 0, GL_FALSE, drawI, GL_READ_ONLY, drawDepthInternalFormat);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, writeDepthBuffers[drawI]);
        {
            const std::size_t workGroupsX = Ceil(viewportW, workGroupSize.x);
            const std::size_t workGroupsY = Ceil(viewportH, workGroupSize.y * 2);
            glDispatchCompute(workGroupsX, workGroupsY, 1);
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        glCopyNamedBufferSubData(writeDepthBuffers[drawI], readDepthBuffers[drawI], 0, 0, sizeof(GLuint));

        // Generate shadows

        // TODO:
        //
        // Variance shadow maps
        //
        // Moment shadow maps
        //
        // Exponential shadow maps
        //
        // PCF

        auto cascadeProperties = GetCascadeProperties(CameraManager::getNearPlane(), CameraManager::getFarPlane(), shadowMapNumCascades, shadowMapRes, CameraManager::getViewMatrix(), CameraManager::getVerticalFOV(), CameraManager::getAspectRatio(), lightView);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
        glViewport(0, 0, shadowMapRes, shadowMapRes);
        glEnable(GL_DEPTH_CLAMP);
        glClear(GL_DEPTH_BUFFER_BIT);

        glProgramUniformMatrix4fv(shadowShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(bunnyModel));
        glProgramUniform1i(shadowShaderProgram, 10, shadowMapNumCascades);
        glProgramUniformMatrix4fv(shadowShaderProgram, 11, shadowMapNumCascades, GL_FALSE, DataPtr(cascadeProperties.transforms));

        glUseProgram(shadowShaderProgram);

        glBindVertexArray(bunnyMesh.VAO);
        glDrawElements(GL_TRIANGLES, bunnyMesh.numIndices, GL_UNSIGNED_INT, nullptr);

        glDisable(GL_DEPTH_CLAMP);
        glViewport(0, 0, viewportW, viewportH);

        // Finish generating shadows

        // Main drawing pass

        glBindFramebuffer(GL_FRAMEBUFFER, drawFramebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDepthFunc(GL_EQUAL);

        glProgramUniformMatrix4fv(lightingShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projView));
        glProgramUniform3fv(lightingShaderProgram, 20, 1, glm::value_ptr(lightDir));
        glProgramUniform1i(lightingShaderProgram, 21, shadowMapNumCascades);
        glProgramUniformMatrix4fv(lightingShaderProgram, 22, shadowMapNumCascades, GL_FALSE, DataPtr(cascadeProperties.transforms));
        glProgramUniform4fv(lightingShaderProgram, 30, 1, glm::value_ptr(cascadeProperties.depths));
        glProgramUniform4fv(lightingShaderProgram, 31, 1, glm::value_ptr(cascadeProperties.sampleSizes));

        glProgramUniform3fv(lightingShaderProgram, 40, 1, glm::value_ptr(camera->cameraPos));

        glUseProgram(lightingShaderProgram);

        glBindTextureUnit(0, shadowMapArray);
        glBindSampler(0, shadowSampler);

        glProgramUniformMatrix4fv(lightingShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(bunnyModel));
        glProgramUniformMatrix3fv(lightingShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(bunnyNormal));
        glBindVertexArray(bunnyMesh.VAO);
        glDrawElements(GL_TRIANGLES, bunnyMesh.numIndices, GL_UNSIGNED_INT, nullptr);

        glProgramUniformMatrix4fv(lightingShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(groundModel));
        glProgramUniformMatrix3fv(lightingShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(groundNormal));
        glBindVertexArray(groundMesh.VAO);
        glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLES, groundMesh.numIndices, GL_UNSIGNED_INT, nullptr);
        glEnable(GL_CULL_FACE);

        glDepthFunc(GL_LESS);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, viewportW, viewportH, 0, 0, viewportW, viewportH, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        drawI = (drawI + 1) % queueSize;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteMeshGLRepr(bunnyMesh);
    deleteMeshGLRepr(groundMesh);
    glDeleteBuffers(writeDepthBuffers.size(), writeDepthBuffers.data());
    glDeleteBuffers(readDepthBuffers.size(), readDepthBuffers.data());
    glDeleteSamplers(1, &shadowSampler);
    glDeleteTextures(1, &shadowMapArray);
    glDeleteTextures(1, &drawDepthTextureArray);
    glDeleteRenderbuffers(1, &drawColorRenderbuffer);
    glDeleteRenderbuffers(1, &drawDepthRenderbuffer);
    glDeleteFramebuffers(1, &shadowFramebuffer);
    glDeleteFramebuffers(1, &drawFramebuffer);
    glDeleteProgram(lightingShaderProgram);
    glDeleteProgram(shadowShaderProgram);
    glDeleteProgram(zPrepassShaderProgram);
    glDeleteProgram(reduceShaderProgram);
    CameraManager::terminate();
}
