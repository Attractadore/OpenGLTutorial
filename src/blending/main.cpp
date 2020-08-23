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
    std::filesystem::path assetsPath = "assets";
    std::filesystem::path meshesPath = assetsPath / "meshes";
    std::filesystem::path texturePath = assetsPath / "textures" / "blending";
    std::filesystem::path shaderSrcPath = assetsPath / "shaders" / "src" / "blending";
    std::filesystem::path shaderBinPath = assetsPath / "shaders" / "bin" / "blending";

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

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    if (glIsEnabled(GL_DEBUG_OUTPUT)) {
        glDebugMessageCallback(debugFunction, nullptr);
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    std::array<GLuint, 2> planeBuffers;
    GLuint& planeVBO = planeBuffers[0];
    GLuint& planeEBO = planeBuffers[1];
    glCreateBuffers(planeBuffers.size(), planeBuffers.data());
    GLuint planeVAO;
    glCreateVertexArrays(1, &planeVAO);
    GLuint numPlaneElements;
    {
        MeshData planeMesh = loadMesh(meshesPath / "transparentplane.obj");
        numPlaneElements = planeMesh.indices.size();
        storeMesh(planeVAO, planeVBO, planeEBO, planeMesh);
    }

    std::array<GLuint, 2> groundBuffers;
    GLuint& groundVBO = groundBuffers[0];
    GLuint& groundEBO = groundBuffers[1];
    glCreateBuffers(groundBuffers.size(), groundBuffers.data());
    GLuint groundVAO;
    glCreateVertexArrays(1, &groundVAO);
    GLuint numGroundElements;
    {
        MeshData groundMesh = loadMesh(meshesPath / "circularplane.obj");
        numGroundElements = groundMesh.indices.size();
        storeMesh(groundVAO, groundVBO, groundEBO, groundMesh);
    }

    GLuint diffuseTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &diffuseTexture);
    loadTexture2D(diffuseTexture, texturePath / ".." / "container2.png", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
    GLuint windowTexture;
    glCreateTextures(GL_TEXTURE_2D, 1, &windowTexture);
    loadTexture2D(windowTexture, texturePath / "window.png", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);

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
        GLuint diffuseVertexShader = createShaderGLSL(GL_VERTEX_SHADER, shaderSrcPath / "diffuse.vert");
        GLuint diffuseFragmentShader = createShaderGLSL(GL_FRAGMENT_SHADER, shaderSrcPath / "diffuse.frag");
        diffuseShaderProgram = createProgram({diffuseVertexShader, diffuseFragmentShader});
    }

    std::vector<glm::vec3> windowPositions = {
        {0.0f, 0.0f, 0.0f},
        {2.0f, 2.0f, 0.0f},
        {-2.0f, 2.0f, 0.0f},
        {2.0f, -2.0f, 0.0f},
        {-2.0f, -2.0f, 0.0f}};
    auto distCompare = [&camera](const glm::vec3& rhs, const glm::vec3& lhs) {
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

        std::sort(windowPositions.begin(), windowPositions.end(), distCompare);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = CameraManager::getViewMatrix();
        glm::mat4 projection = CameraManager::getProjectionMatrix();

        glProgramUniformMatrix4fv(diffuseShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projection));
        glProgramUniformMatrix4fv(diffuseShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));

        glUseProgram(diffuseShaderProgram);

        glBindVertexArray(cubeVAO);
        glBindTextureUnit(0, diffuseTexture);
        glBindSampler(0, clampSampler);
        for (const auto& cubePos: cubePositions) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePos);
            glProgramUniformMatrix4fv(diffuseShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, numCubeElements, GL_UNSIGNED_INT, nullptr);
        }
        glBindVertexArray(groundVAO);
        glBindSampler(0, wrapSampler);
        {
            glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -1.0f}), glm::vec3(0.3f));
            glProgramUniformMatrix4fv(diffuseShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, numGroundElements, GL_UNSIGNED_INT, nullptr);
        }

        glEnable(GL_BLEND);
        glBindVertexArray(planeVAO);
        glBindTextureUnit(0, windowTexture);
        glBindSampler(0, clampSampler);
        for (const auto& windowPos: windowPositions) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), windowPos);
            glProgramUniformMatrix4fv(diffuseShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, numPlaneElements, GL_UNSIGNED_INT, nullptr);
        }
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
