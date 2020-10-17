#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "debug.hpp"
#include "load_model.hpp"
#include "load_shader.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    glm::vec3 v0 = projToWorld(projViewInv, {-1.0f, -1.0f,  1.0f});
    glm::vec3 v1 = projToWorld(projViewInv, { 1.0f,  1.0f, -1.0f});
    glm::vec3 v2 = projToWorld(projViewInv, { 1.0f,  1.0f,  1.0f});

    float r1 = glm::length(v1 - v0);
    float r2 = glm::length(v2 - v0);
    
    return glm::max(r1, r2);
}

float discretize(float v, float step) {
    return glm::floor(v / step) * step;
}

glm::mat4 getLightProj(glm::mat4 const& lightView, glm::mat4 const& cameraProjViewInv, float shadowSampleSize, uint32_t shadowMapRes) {
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
    float boxSize = (shadowMapRes + 1) * shadowSampleSize;
    maxBox.x = minBox.x + boxSize;
    maxBox.y = minBox.y + boxSize;

    glm::mat4 lightProj = glm::ortho(minBox.x, maxBox.x, minBox.y, maxBox.y, minBox.z, maxBox.z);

    return lightProj;
}

int main() {
    std::filesystem::path assetsPath = "assets";
    std::filesystem::path meshesPath = assetsPath / "meshes" / "shadows";
    std::filesystem::path texturePath = assetsPath / "textures" / "shadows";
    std::filesystem::path shaderSrcPath = assetsPath / "shaders" / "src" / "shadows";
    std::filesystem::path shaderBinPath = assetsPath / "shaders" / "bin" / "shadows";

    int viewportW = 1280;
    int viewportH = 720;

    float cameraSpeed = 5.0f;
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

    MeshGLRepr bunnyMesh = createMeshGLRepr(meshesPath / "bunny.obj");
    glm::mat4 bunnyModel{1.0f};
    glm::mat3 bunnyNormal{1.0f};
    MeshGLRepr groundMesh = createMeshGLRepr(meshesPath / ".." / "circularplane.obj");
    glm::mat4 groundModel = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f});
    glm::mat3 groundNormal{1.0f};

    GLuint lightingShaderProgram;
    {
        GLuint lightingVertexShader = createShaderGLSL(GL_VERTEX_SHADER, shaderSrcPath / "lighting.vert");
        GLuint lightingFragmentShader = createShaderGLSL(GL_FRAGMENT_SHADER, shaderSrcPath / "lighting.frag");
        lightingShaderProgram = createProgram({lightingVertexShader, lightingFragmentShader});
        glDeleteShader(lightingVertexShader);
        glDeleteShader(lightingFragmentShader);
    }
    GLuint shadowShaderProgram;
    {
        GLuint shadowVertexShader = createShaderGLSL(GL_VERTEX_SHADER, shaderSrcPath / "shadow.vert");
        shadowShaderProgram = createProgram({shadowVertexShader});
        glDeleteShader(shadowVertexShader);
    }

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

    uint32_t shadow_map_res = 2048;
    GLuint shadowMap;
    glCreateTextures(GL_TEXTURE_2D, 1, &shadowMap);
    glTextureStorage2D(shadowMap, 1, GL_DEPTH_COMPONENT32, shadow_map_res, shadow_map_res);

    GLuint shadowFramebuffer;
    glCreateFramebuffers(1, &shadowFramebuffer);
    glNamedFramebufferTexture(shadowFramebuffer, GL_DEPTH_ATTACHMENT, shadowMap, 0);
    glNamedFramebufferDrawBuffer(shadowFramebuffer, GL_NONE);
    glNamedFramebufferReadBuffer(shadowFramebuffer, GL_NONE);

    double currentTime = 0;
    double previousTime;
    auto window = CameraManager::getWindow();
    while (!glfwWindowShouldClose(window)) {
        previousTime = currentTime;
        currentTime = glfwGetTime();
        float deltaTime = currentTime - previousTime;

        CameraManager::processEvents();

        glm::vec3 cameraMovementInput = CameraManager::getCameraMovementInput();
        if (glm::length(cameraMovementInput) > 0.0f) {
            cameraMovementInput = glm::normalize(cameraMovementInput);
            camera->cameraPos += (deltaTime * cameraSpeed) * cameraMovementInput;
        }

        glm::mat4 projView = CameraManager::getProjectionMatrix() * CameraManager::getViewMatrix();
        glm::mat4 projViewInv = glm::inverse(projView);

        // Generate shadows
        
        // TODO:
        // CSM algorithm:
        // 1) Partition view frustrum
        // 2) Calculate bounding boxes
        // 3) Use bounding boxes and view frustrum partitions to calculate shadow map texture space transform matrices
        //
        // Variance shadow maps
        //
        // Moment shadow maps
        //
        // Exponential shadow maps
        //
        // PCF

        float shadowSampleSize = getFrustrumDiag(projViewInv) / shadow_map_res;
        glm::mat4 lightProj = getLightProj(lightView, projViewInv, shadowSampleSize, shadow_map_res);
        glm::mat4 lightProjView = lightProj * lightView;

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
        glViewport(0, 0, shadow_map_res, shadow_map_res);
        glEnable(GL_DEPTH_CLAMP);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(shadowShaderProgram);
        glProgramUniformMatrix4fv(shadowShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(lightProjView));

        glProgramUniformMatrix4fv(shadowShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(bunnyModel));
        glBindVertexArray(bunnyMesh.VAO);
        glDrawElements(GL_TRIANGLES, bunnyMesh.numIndices, GL_UNSIGNED_INT, nullptr);

        glDisable(GL_DEPTH_CLAMP);
        glViewport(0, 0, viewportW, viewportH);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Finish generating shadows

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glProgramUniform3fv(lightingShaderProgram, 20, 1, glm::value_ptr(lightDir));
        glProgramUniformMatrix4fv(lightingShaderProgram, 21, 1, GL_FALSE, glm::value_ptr(lightProjView));
        glProgramUniform1f(lightingShaderProgram, 22, shadowSampleSize);
        glProgramUniform3fv(lightingShaderProgram, 30, 1, glm::value_ptr(camera->cameraPos));

        glUseProgram(lightingShaderProgram);
        glBindTextureUnit(0, shadowMap);
        glBindSampler(0, shadowSampler);

        glProgramUniformMatrix4fv(lightingShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projView));
        glProgramUniformMatrix4fv(lightingShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(bunnyModel));
        glProgramUniformMatrix3fv(lightingShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(bunnyNormal));
        glBindVertexArray(bunnyMesh.VAO);
        glDrawElements(GL_TRIANGLES, bunnyMesh.numIndices, GL_UNSIGNED_INT, nullptr);

        glProgramUniformMatrix4fv(lightingShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projView));
        glProgramUniformMatrix4fv(lightingShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(groundModel));
        glProgramUniformMatrix3fv(lightingShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(groundNormal));
        glBindVertexArray(groundMesh.VAO);
        glDisable(GL_CULL_FACE);
        glDrawElements(GL_TRIANGLES, groundMesh.numIndices, GL_UNSIGNED_INT, nullptr);
        glEnable(GL_CULL_FACE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteMeshGLRepr(bunnyMesh);
    deleteMeshGLRepr(groundMesh);
    glDeleteSamplers(1, &shadowSampler);
    glDeleteTextures(1, &shadowMap);
    glDeleteFramebuffers(1, &shadowFramebuffer);
    glDeleteProgram(lightingShaderProgram);
    glDeleteProgram(shadowShaderProgram);
    CameraManager::terminate();
}
