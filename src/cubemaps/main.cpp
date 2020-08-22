#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "debug.hpp"
#include "load_model.hpp"
#include "load_shader.hpp"
#include "load_texture.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

int main() {
    std::filesystem::path assetsPath = "assets";
    std::filesystem::path meshesPath = assetsPath / "meshes";
    std::filesystem::path texturePath = assetsPath / "textures" / "cubemaps";
    std::filesystem::path shaderSrcPath = assetsPath / "shaders" / "src" / "cubemaps";
    std::filesystem::path shaderBinPath = assetsPath / "shaders" / "bin" / "cubemaps";

    int viewportW = 1280;
    int viewportH = 720;

    float cameraSpeed = 5.0f;
    glm::vec3 cameraStartPos = {0.0f, 0.0f, 5.0f};
    glm::vec3 cameraStartLookDir = {0.0f, 0.0f, -1.0f};
    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDir, glm::vec3{0.0f, -1.0f, 0.0f});

    CameraManager::initialize(viewportW, viewportH);
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
    glEnable(GL_FRAMEBUFFER_SRGB);

    std::array<GLuint, 2> cubeBuffers;
    GLuint& cubeVBO = cubeBuffers[0];
    GLuint& cubeEBO = cubeBuffers[1];
    glCreateBuffers(cubeBuffers.size(), cubeBuffers.data());
    GLuint cubeVAO;
    glCreateVertexArrays(1, &cubeVAO);
    GLuint numCubeElements;
    {
        MeshData cubeMesh = loadMesh(meshesPath / "cube.obj");
        numCubeElements = cubeMesh.indices.size();
        storeMesh(cubeVAO, cubeVBO, cubeEBO, cubeMesh);
    }

    GLuint skyboxCubeMap;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &skyboxCubeMap);
    loadTextureCubeMap(skyboxCubeMap,
                       {texturePath / "skybox/right.png",
                        texturePath / "skybox/left.png",
                        texturePath / "skybox/bottom.png",
                        texturePath / "skybox/top.png",
                        texturePath / "skybox/back.png",
                        texturePath / "skybox/front.png"},
                       GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
    GLuint skyboxSampler;
    glCreateSamplers(1, &skyboxSampler);
    glSamplerParameteri(skyboxSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(skyboxSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(skyboxSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(skyboxSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(skyboxSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    GLuint skyboxShaderProgram;
    {
        GLuint skyboxVertexShader = createShaderGLSL(GL_VERTEX_SHADER, shaderSrcPath / "skybox.vert");
        GLuint skyboxFragmentShader = createShaderGLSL(GL_FRAGMENT_SHADER, shaderSrcPath / "skybox.frag");
        skyboxShaderProgram = createProgram({skyboxVertexShader, skyboxFragmentShader});
    }
    GLuint mirrorBoxShaderProgram;
    {
        GLuint mirrorBoxVertexShader = createShaderGLSL(GL_VERTEX_SHADER, shaderSrcPath / "mirror.vert");
        GLuint mirrorBoxFragmentShader = createShaderGLSL(GL_FRAGMENT_SHADER, shaderSrcPath / "mirror.frag");
        mirrorBoxShaderProgram = createProgram({mirrorBoxVertexShader, mirrorBoxFragmentShader});
    }

    glm::vec3 cameraMovementInput;
    double currentTime = 0;
    double previousTime;
    auto window = CameraManager::getWindow();
    while (!glfwWindowShouldClose(window)) {
        previousTime = currentTime;
        currentTime = glfwGetTime();
        float deltaTime = currentTime - previousTime;

        CameraManager::processEvents();

        cameraMovementInput = glm::vec3(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraMovementInput += camera->getCameraForwardVector();
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraMovementInput -= camera->getCameraForwardVector();
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraMovementInput += camera->getCameraRightVector();
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraMovementInput -= camera->getCameraRightVector();
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            cameraMovementInput += camera->getCameraUpVector();
        }
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            cameraMovementInput -= camera->getCameraUpVector();
        }

        if (glm::length(cameraMovementInput) > 0.0f) {
            cameraMovementInput = glm::normalize(cameraMovementInput);
            camera->cameraPos += (deltaTime * cameraSpeed) * cameraMovementInput;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = CameraManager::getViewMatrix();
        glm::mat4 projection = CameraManager::getProjectionMatrix();

        glm::mat4 mirrorCubeModel = glm::rotate(glm::mat4(1.0f), float(currentTime / 10.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        glm::mat3 mirrorCubeNormal = glm::mat3(mirrorCubeModel);

        glProgramUniformMatrix4fv(mirrorBoxShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projection));
        glProgramUniformMatrix4fv(mirrorBoxShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));
        glProgramUniformMatrix4fv(mirrorBoxShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(mirrorCubeModel));
        glProgramUniformMatrix3fv(mirrorBoxShaderProgram, 3, 1, GL_FALSE, glm::value_ptr(mirrorCubeNormal));
        glProgramUniform3fv(mirrorBoxShaderProgram, 4, 1, glm::value_ptr(camera->cameraPos));

        view[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        glProgramUniformMatrix4fv(skyboxShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projection));
        glProgramUniformMatrix4fv(skyboxShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));

        glBindTextureUnit(0, skyboxCubeMap);
        glBindSampler(0, skyboxSampler);

        glUseProgram(mirrorBoxShaderProgram);
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, numCubeElements, GL_UNSIGNED_INT, nullptr);
        glUseProgram(skyboxShaderProgram);
        glDrawElements(GL_TRIANGLES, numCubeElements, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
