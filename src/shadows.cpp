#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "debug.hpp"
#include "load_model.hpp"
#include "load_shader.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

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

std::vector<float> Partition(const float nearPlane, const float farPlane, const std::size_t numCascades) {
    std::vector<float> cascadeBounds(numCascades + 1);
    const float cascadeScale = std::pow(farPlane / nearPlane, 1.0f / numCascades);
    float cascadeNearPlane = nearPlane;
    for (std::uint8_t i = 0; i < numCascades; i++) {
        cascadeBounds[i] = cascadeNearPlane;
        cascadeNearPlane *= cascadeScale;
    }
    cascadeBounds[numCascades] = farPlane;
    return cascadeBounds;
}

float DistanceToDepth(float distance, float nearPlane, float farPlane) {
    return (distance - nearPlane) / (farPlane - nearPlane) * farPlane / distance;
}

float DepthToDistace(float depth, float near_plane, float far_plane) {
    return 1.0F / glm::mix(1.0F / near_plane, 1.0F / far_plane, depth);
}

struct CascadeProperties {
    std::vector<glm::mat4> transforms;
    glm::vec4 sampleSizes, depths;
};

CascadeProperties GetCascadeProperties(float cascades_near_plane, float cascades_far_plane, std::uint8_t numCascades, std::uint16_t shadowRes, glm::mat4 const& m4View, float camera_near_plane, float camera_far_plane, float verticalFOV, float aspectRatio, glm::mat4 const& lightView) {
    CascadeProperties properties;
    const auto cascadeBounds = Partition(cascades_near_plane, cascades_far_plane, numCascades);
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
        const float nDepth = DistanceToDepth(n, camera_near_plane, camera_far_plane);
        properties.depths[i] = nDepth;
    }
    return properties;
}

template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
T const* DataPtr(std::vector<glm::mat<C, R, T, Q>> const& data) {
    return reinterpret_cast<T const*>(data.data());
}

std::size_t CeilDiv(std::size_t x, std::size_t y) {
    return x / y + (x % y != 0);
}

enum class ImpDrawTypes {
    DepthOnly,
    Shadows,
    Color
};

template <ImpDrawTypes drawType>
void ImpDrawMesh(GLuint program, glm::mat4 const& projView, MeshGLRepr const* mesh) {
    if constexpr (drawType == ImpDrawTypes::DepthOnly) {
        glProgramUniformMatrix4fv(program, 0, 1, GL_FALSE, glm::value_ptr(projView * mesh->model));
    } else if constexpr (drawType == ImpDrawTypes::Shadows) {
        glProgramUniformMatrix4fv(program, 0, 1, GL_FALSE, glm::value_ptr(mesh->model));
    } else if constexpr (drawType == ImpDrawTypes::Color) {
        glProgramUniformMatrix4fv(program, 0, 1, GL_FALSE, glm::value_ptr(projView * mesh->model));
        glProgramUniformMatrix4fv(program, 1, 1, GL_FALSE, glm::value_ptr(mesh->model));
        glProgramUniformMatrix3fv(program, 2, 1, GL_FALSE, glm::value_ptr(mesh->normal));
    }

    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, nullptr);
}

template <ImpDrawTypes drawType>
void ImpDrawMeshes(GLuint program, glm::mat4 const& projView, std::vector<MeshGLRepr*>& meshes) {
    auto meshesEnd = [&meshes]() {
        if constexpr (drawType == ImpDrawTypes::Shadows) {
            return std::partition(meshes.begin(), meshes.end(), [](MeshGLRepr const* mesh) { return mesh->bCastsShadows; });
        }
        return meshes.end();
    }();

    auto unculledMeshesBegin = std::partition(meshes.begin(), meshesEnd, [](MeshGLRepr const* mesh) { return mesh->bCullFaces; });

    glUseProgram(program);

    glEnable(GL_CULL_FACE);
    for (auto it = meshes.begin(); it < unculledMeshesBegin; ++it) {
        ImpDrawMesh<drawType>(program, projView, *it);
    }

    glDisable(GL_CULL_FACE);
    for (auto it = unculledMeshesBegin; it < meshesEnd; ++it) {
        ImpDrawMesh<drawType>(program, projView, *it);
    }
}

void DrawMeshesDepthOnly(GLuint program, glm::mat4 const& projView, std::vector<MeshGLRepr*>& meshes) {
    ImpDrawMeshes<ImpDrawTypes::DepthOnly>(program, projView, meshes);
}

void DrawMeshShadows(GLuint program, std::vector<MeshGLRepr*>& meshes) {
    ImpDrawMeshes<ImpDrawTypes::Shadows>(program, {}, meshes);
}

void DrawMeshesColor(GLuint program, glm::mat4 const& projView, std::vector<MeshGLRepr*>& meshes) {
    ImpDrawMeshes<ImpDrawTypes::Color>(program, projView, meshes);
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

    MeshGLRepr bunnyMesh = createMeshGLRepr(meshesPath + "/bunny.obj");
    bunnyMesh.bCullFaces = true;
    bunnyMesh.bCastsShadows = true;
    MeshGLRepr groundMesh = createMeshGLRepr(meshesPath + "/../circularplane.obj");
    groundMesh.model = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f});
    std::vector<MeshGLRepr*> meshes = {&bunnyMesh, &groundMesh};

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
    GLenum drawDepthInternalFormat = GL_RG16;
    GLenum drawDepthFormat = GL_RG;
    glTextureStorage3D(drawDepthTextureArray, 1, drawDepthInternalFormat, viewportW, viewportH, queueSize);

    std::vector<GLuint> readDepthBuffers(2 * queueSize);
    glCreateBuffers(readDepthBuffers.size(), readDepthBuffers.data());
    for (const auto& readDepthBuffer: readDepthBuffers) {
        // GL_CLIENT_STORAGE_BIT alone is not enough to get cpu side storage
        glNamedBufferStorage(readDepthBuffer, sizeof(GLuint[2]), nullptr, GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT);
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

        // Get min and max depth

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        glm::uvec2 frame_depths;
        glGetNamedBufferSubData(readDepthBuffers[drawI], 0, sizeof(frame_depths), glm::value_ptr(frame_depths));
        {
            constexpr GLuint clear_min_depth = -1U;
            constexpr GLuint clear_max_depth = 0;
            glClearNamedBufferSubData(readDepthBuffers[drawI], drawDepthInternalFormat, 0, sizeof(GLuint), drawDepthFormat, GL_UNSIGNED_INT, &clear_min_depth);
            glClearNamedBufferSubData(readDepthBuffers[drawI], drawDepthInternalFormat, sizeof(GLuint), sizeof(GLuint), drawDepthFormat, GL_UNSIGNED_INT, &clear_max_depth);
        }

        glm::mat4 projView = CameraManager::getProjectionMatrix() * CameraManager::getViewMatrix();

        // Do z prepass

        glBindFramebuffer(GL_FRAMEBUFFER, drawFramebuffer);
        glNamedFramebufferTextureLayer(drawFramebuffer, GL_COLOR_ATTACHMENT1, drawDepthTextureArray, 0, drawI);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        glReadBuffer(GL_COLOR_ATTACHMENT1);

        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        DrawMeshesDepthOnly(zPrepassShaderProgram, projView, meshes);

        glNamedFramebufferTextureLayer(drawFramebuffer, GL_COLOR_ATTACHMENT1, 0, 0, 0);

        // Dispatch compute for min depth

        glUseProgram(reduceShaderProgram);
        glProgramUniform2ui(reduceShaderProgram, 0, viewportW, viewportH);
        glBindImageTexture(0, drawDepthTextureArray, 0, GL_FALSE, drawI, GL_READ_ONLY, drawDepthInternalFormat);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, readDepthBuffers[drawI]);
        {
            const std::size_t workGroupsX = CeilDiv(viewportW, workGroupSize.x);
            const std::size_t workGroupsY = CeilDiv(viewportH, workGroupSize.y);
            glDispatchCompute(workGroupsX, workGroupsY, 1);
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

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

        const glm::vec2 f_frame_depths = glm::vec2(frame_depths) / float((1 << 16) - 1);
        const float f_min_depth = f_frame_depths[0];
        const float f_max_depth = f_frame_depths[1];
        const float cameraBias = queueSize * cameraSpeed / 30.0f;
        const float cascades_near_plane = std::max(DepthToDistace(f_min_depth, CameraManager::getNearPlane(), CameraManager::getFarPlane()) - cameraBias,
                                                   CameraManager::getNearPlane());
        const float cascades_far_plane = std::min(DepthToDistace(f_max_depth, CameraManager::getNearPlane(), CameraManager::getFarPlane()) + cameraBias,
                                                  CameraManager::getFarPlane());
        //std::cout << cascades_near_plane << ' ' << cascades_far_plane << '\n';

        auto cascadeProperties = GetCascadeProperties(cascades_near_plane, cascades_far_plane, shadowMapNumCascades, shadowMapRes,
                                                      CameraManager::getViewMatrix(),
                                                      CameraManager::getNearPlane(), CameraManager::getFarPlane(),
                                                      CameraManager::getVerticalFOV(), CameraManager::getAspectRatio(), lightView);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
        glViewport(0, 0, shadowMapRes, shadowMapRes);
        glEnable(GL_DEPTH_CLAMP);
        glClear(GL_DEPTH_BUFFER_BIT);

        glProgramUniform1i(shadowShaderProgram, 10, shadowMapNumCascades);
        glProgramUniformMatrix4fv(shadowShaderProgram, 11, shadowMapNumCascades, GL_FALSE, DataPtr(cascadeProperties.transforms));

        DrawMeshShadows(shadowShaderProgram, meshes);

        glDisable(GL_DEPTH_CLAMP);
        glViewport(0, 0, viewportW, viewportH);

        // Finish generating shadows

        // Main drawing pass

        glBindFramebuffer(GL_FRAMEBUFFER, drawFramebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glProgramUniformMatrix4fv(lightingShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projView));
        glProgramUniform3fv(lightingShaderProgram, 20, 1, glm::value_ptr(lightDir));
        glProgramUniform1i(lightingShaderProgram, 21, shadowMapNumCascades);
        glProgramUniformMatrix4fv(lightingShaderProgram, 22, shadowMapNumCascades, GL_FALSE, DataPtr(cascadeProperties.transforms));
        glProgramUniform4fv(lightingShaderProgram, 30, 1, glm::value_ptr(cascadeProperties.depths));
        glProgramUniform4fv(lightingShaderProgram, 31, 1, glm::value_ptr(cascadeProperties.sampleSizes));
        glProgramUniform3fv(lightingShaderProgram, 40, 1, glm::value_ptr(camera->cameraPos));

        glBindTextureUnit(0, shadowMapArray);
        glBindSampler(0, shadowSampler);

        DrawMeshesColor(lightingShaderProgram, projView, meshes);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, viewportW, viewportH, 0, 0, viewportW, viewportH, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        drawI = (drawI + 1) % queueSize;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteMeshGLRepr(bunnyMesh);
    deleteMeshGLRepr(groundMesh);
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
