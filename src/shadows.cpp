#include "Camera.hpp"
#include "CameraManager.hpp"
#include "assets/shaders/src/shadows/indices.glsl"
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

enum class ImpDrawTypes {
    DepthOnly,
    Shadows,
    Color
};

template <ImpDrawTypes drawType>
void ImpDrawMesh(GLuint program, glm::mat4 const& projView, MeshGLRepr const* mesh) {
    if constexpr (drawType == ImpDrawTypes::DepthOnly) {
        glProgramUniformMatrix4fv(program, SHADOW_VERT_TRANSFORM_LOCATION, 1, GL_FALSE, glm::value_ptr(projView * mesh->model));
    } else if constexpr (drawType == ImpDrawTypes::Shadows) {
        glProgramUniformMatrix4fv(program, SHADOW_VERT_TRANSFORM_LOCATION, 1, GL_FALSE, glm::value_ptr(mesh->model));
    } else if constexpr (drawType == ImpDrawTypes::Color) {
        glProgramUniformMatrix4fv(program, LIGHTING_VERT_PROJ_VIEW_MODEL_LOCATION, 1, GL_FALSE, glm::value_ptr(projView * mesh->model));
        glProgramUniformMatrix4fv(program, LIGHTING_VERT_MODEL_LOCATION, 1, GL_FALSE, glm::value_ptr(mesh->model));
        glProgramUniformMatrix3fv(program, LIGHTING_VERT_NORMAL_LOCATION, 1, GL_FALSE, glm::value_ptr(mesh->normal));
    }

    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, nullptr);
}

template <ImpDrawTypes drawType>
void ImpDrawMeshes(GLuint program, glm::mat4 const& projView, std::vector<MeshGLRepr*>& meshes) {
    auto unculledMeshesBegin = std::partition(meshes.begin(), meshes.end(), [](MeshGLRepr const* mesh) { return mesh->bCullFaces; });

    glUseProgram(program);

    auto it = meshes.cbegin();
    glEnable(GL_CULL_FACE);
    for (; it < unculledMeshesBegin; ++it) {
        ImpDrawMesh<drawType>(program, projView, *it);
    }
    glDisable(GL_CULL_FACE);
    for (; it < meshes.cend(); ++it) {
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
    gladLoadGL(glfwGetProcAddress);

    activateGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);

    MeshGLRepr bunnyMesh = createMeshGLRepr(meshesPath + "/bunny.obj");
    bunnyMesh.bCullFaces = true;
    MeshGLRepr bunny_shadow_mesh = createMeshGLRepr(meshesPath + "/bunny_shadow.obj");
    bunny_shadow_mesh.bCullFaces = true;
    MeshGLRepr groundMesh = createMeshGLRepr(meshesPath + "/../circularplane.obj");
    groundMesh.model = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f});
    std::vector<MeshGLRepr*> meshes = {&bunnyMesh, &groundMesh};
    std::vector<MeshGLRepr*> shadow_meshes = {&bunny_shadow_mesh};

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
    GLuint z_partition_shader_program = [&shaderBinPath]() {
        GLuint z_partition_compute_shader = createShaderSPIRV(GL_COMPUTE_SHADER, shaderBinPath + "/ZPartition.comp.spv");
        GLuint z_partition_shader_program = createProgram({z_partition_compute_shader});
        glDeleteShader(z_partition_compute_shader);
        return z_partition_shader_program;
    }();

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

    unsigned shadowMapRes = 1024;
    GLuint shadowMapArray;
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &shadowMapArray);
    glTextureStorage3D(shadowMapArray, 1, GL_DEPTH_COMPONENT32, shadowMapRes, shadowMapRes, 4);

    GLuint shadowFramebuffer = 0;
    glCreateFramebuffers(1, &shadowFramebuffer);
    glNamedFramebufferTexture(shadowFramebuffer, GL_DEPTH_ATTACHMENT, shadowMapArray, 0);
    glNamedFramebufferDrawBuffer(shadowFramebuffer, GL_NONE);
    glNamedFramebufferReadBuffer(shadowFramebuffer, GL_NONE);

    GLuint depthComputeBuffer = 0;
    glCreateBuffers(1, &depthComputeBuffer);
    glNamedBufferStorage(depthComputeBuffer, sizeof(GLuint[2]), nullptr, 0);
    GLuint zPartitionBuffer = 0;
    glCreateBuffers(1, &zPartitionBuffer);
    glNamedBufferStorage(zPartitionBuffer, sizeof(glm::mat4[4]) + sizeof(glm::vec4[2]), nullptr, 0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DEPTH_SSBO_BINDING, depthComputeBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, CASCADE_SSBO_BINDING, zPartitionBuffer);

    glBindTextureUnit(LIGHTING_FRAG_SHADOW_MAP_ARR_BINDING, shadowMapArray);
    glBindSampler(LIGHTING_FRAG_SHADOW_MAP_ARR_BINDING, shadowSampler);

    bool b_shadow_cascade_depth_adjust = false;
    bool b_shadow_texel_move = true;

    glProgramUniform1i(zPrepassShaderProgram, B_SHADOW_CASCADE_DEPTH_ADJUST_LOCATION, b_shadow_cascade_depth_adjust);

    glProgramUniform1ui(z_partition_shader_program, Z_PARTITION_COMP_SHADOW_MAP_RES_LOCATION, shadowMapRes);
    glProgramUniform1f(z_partition_shader_program, Z_PARTITION_COMP_CAMERA_NEAR_PLANE_LOCATION, CameraManager::getNearPlane());
    glProgramUniform1f(z_partition_shader_program, Z_PARTITION_COMP_CAMERA_FAR_PLANE_LOCATION, CameraManager::getFarPlane());
    glProgramUniformMatrix4fv(z_partition_shader_program, Z_PARTITION_COMP_LIGHT_VIEW_LOCATION, 1, GL_FALSE, glm::value_ptr(lightView));
    glProgramUniform1i(z_partition_shader_program, B_SHADOW_CASCADE_DEPTH_ADJUST_LOCATION, b_shadow_cascade_depth_adjust);
    glProgramUniform1i(z_partition_shader_program, B_SHADOW_TEXEL_MOVE_LOCATION, b_shadow_texel_move);

    glProgramUniform3fv(lightingShaderProgram, LIGHTING_FRAG_LIGHT_SHINE_DIR_LOCATION, 1, glm::value_ptr(lightDir));

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

        const glm::mat4 projView = CameraManager::getProjectionMatrix() * CameraManager::getViewMatrix();

        // Do z prepass

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        glClear(GL_DEPTH_BUFFER_BIT);

        DrawMeshesDepthOnly(zPrepassShaderProgram, projView, meshes);

        glUseProgram(z_partition_shader_program);
        glUniformMatrix4fv(Z_PARTITION_COMP_PROJ_VIEW_INV_LOCATION, 1, GL_FALSE, glm::value_ptr(glm::inverse(projView)));

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDispatchCompute(1, 1, 1);

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

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
        glViewport(0, 0, shadowMapRes, shadowMapRes);
        glEnable(GL_DEPTH_CLAMP);
        glClear(GL_DEPTH_BUFFER_BIT);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        DrawMeshShadows(shadowShaderProgram, shadow_meshes);

        glDisable(GL_DEPTH_CLAMP);
        glViewport(0, 0, viewportW, viewportH);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Main drawing pass

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glProgramUniform3fv(lightingShaderProgram, LIGHTING_FRAG_CAMERA_POS_LOCATION, 1, glm::value_ptr(camera->cameraPos));

        DrawMeshesColor(lightingShaderProgram, projView, meshes);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteMeshGLRepr(bunnyMesh);
    deleteMeshGLRepr(groundMesh);
    deleteMeshGLRepr(bunny_shadow_mesh);
    glDeleteBuffers(1, &depthComputeBuffer);
    glDeleteBuffers(1, &zPartitionBuffer);
    glDeleteSamplers(1, &shadowSampler);
    glDeleteTextures(1, &shadowMapArray);
    glDeleteFramebuffers(1, &shadowFramebuffer);
    glDeleteProgram(lightingShaderProgram);
    glDeleteProgram(shadowShaderProgram);
    glDeleteProgram(zPrepassShaderProgram);
    glDeleteProgram(z_partition_shader_program);
    CameraManager::terminate();
}
