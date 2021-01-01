#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "debug.hpp"
#include "load_model.hpp"
#include "load_shader.hpp"
#include "load_texture.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

int main() {
    const std::filesystem::path assetsPath = "assets";
    const std::filesystem::path meshesPath = assetsPath / "meshes";
    const std::filesystem::path texturePath = assetsPath / "textures" / "blending";
    const std::filesystem::path shaderBinPath = assetsPath / "shaders" / "bin" / "blending";

    int viewportW = 1280;
    int viewportH = 720;

    float cameraSpeed = 5.0f;
    glm::vec3 cameraStartPos = {0.0f, -10.0f, 4.0f};
    glm::vec3 cameraStartLookDir = -cameraStartPos;
    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDir, glm::vec3{0.0f, 0.0f, 1.0f});

    CameraManager::initialize(viewportW, viewportH);
    CameraManager::currentCamera = camera;
    CameraManager::enableCameraLook();
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    activateGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    MeshGLRepr cubeMesh = createMeshGLRepr(meshesPath / "cube.obj");
    MeshGLRepr planeMesh = createMeshGLRepr(meshesPath / "transparentplane.obj");
    MeshGLRepr groundMesh = createMeshGLRepr(meshesPath / "circularplane.obj");

    GLuint diffuseTexture = createTexture2D(texturePath / ".." / "container2.png", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
    GLuint windowTexture = createTexture2D(texturePath / "window.png", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);

    GLuint clampSampler;
    glCreateSamplers(1, &clampSampler);
    glSamplerParameteri(clampSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(clampSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(clampSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(clampSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLuint wrapSampler;
    glCreateSamplers(1, &wrapSampler);
    glSamplerParameteri(wrapSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(wrapSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(wrapSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(wrapSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint diffuseShaderProgram;
    {
        GLuint diffuseVertexShader = createShaderSPIRV(GL_VERTEX_SHADER, shaderBinPath / "diffuse.vert.spv");
        GLuint diffuseFragmentShader = createShaderSPIRV(GL_FRAGMENT_SHADER, shaderBinPath / "diffuse.frag.spv");
        diffuseShaderProgram = createProgram({diffuseVertexShader, diffuseFragmentShader});
        glDeleteShader(diffuseVertexShader);
        glDeleteShader(diffuseFragmentShader);
    }

    std::vector<glm::vec3> windowPositions = {
        {0.0f, 0.0f, 0.0f},
        {2.0f, 2.0f, 0.0f},
        {-2.0f, 2.0f, 0.0f},
        {2.0f, -2.0f, 0.0f},
        {-2.0f, -2.0f, 0.0f}};
    auto distSort = [&camera](const glm::vec3& rhs, const glm::vec3& lhs) {
        glm::vec3 rhsDist = rhs - camera->cameraPos;
        glm::vec3 lhsDist = lhs - camera->cameraPos;
        return glm::dot(rhsDist, rhsDist) > glm::dot(lhsDist, lhsDist);
    };

    std::vector<glm::vec3> cubePositions = {
        {5.0f, 3.0f, 0.0f},
        {-5.0f, 3.0f, 0.0f},
        {5.0f, -3.0f, 0.0f},
        {-5.0f, -3.0f, 0.0f}};

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

        std::sort(windowPositions.begin(), windowPositions.end(), distSort);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = CameraManager::getViewMatrix();
        glm::mat4 projection = CameraManager::getProjectionMatrix();

        glProgramUniformMatrix4fv(diffuseShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projection));
        glProgramUniformMatrix4fv(diffuseShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));

        glUseProgram(diffuseShaderProgram);

        glBindVertexArray(cubeMesh.VAO);
        glBindTextureUnit(0, diffuseTexture);
        glBindSampler(0, clampSampler);
        for (const auto& cubePos: cubePositions) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePos);
            glProgramUniformMatrix4fv(diffuseShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, cubeMesh.numIndices, GL_UNSIGNED_INT, nullptr);
        }
        glBindVertexArray(groundMesh.VAO);
        glBindSampler(0, wrapSampler);
        {
            glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -1.0f}), glm::vec3(0.3f));
            glProgramUniformMatrix4fv(diffuseShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, groundMesh.numIndices, GL_UNSIGNED_INT, nullptr);
        }

        glEnable(GL_BLEND);
        glBindVertexArray(planeMesh.VAO);
        glBindTextureUnit(0, windowTexture);
        glBindSampler(0, clampSampler);
        for (const auto& windowPos: windowPositions) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), windowPos);
            glProgramUniformMatrix4fv(diffuseShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, planeMesh.numIndices, GL_UNSIGNED_INT, nullptr);
        }
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(diffuseShaderProgram);
    glDeleteSamplers(1, &clampSampler);
    glDeleteSamplers(1, &wrapSampler);
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &windowTexture);
    deleteMeshGLRepr(cubeMesh);
    deleteMeshGLRepr(planeMesh);
    deleteMeshGLRepr(groundMesh);
    CameraManager::terminate();
}
