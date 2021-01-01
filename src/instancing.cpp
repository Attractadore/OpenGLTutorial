#include "glad.h"

#include "Camera.hpp"
#include "CameraManager.hpp"
#include "debug.hpp"
#include "load_model.hpp"
#include "load_shader.hpp"
#include "util.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <random>

int main() {
    const std::filesystem::path assetsPath = "assets";
    const std::filesystem::path meshesPath = assetsPath / "meshes";
    const std::filesystem::path shaderBinPath = assetsPath / "shaders" / "bin" / "instancing";

    float cameraSpeed = 500.0f;
    glm::vec3 cameraStartPos = {-3500.0f, 0.0f, 1500.0f};
    glm::vec3 cameraStartLookDir = -cameraStartPos;
    auto camera = std::make_shared<Camera>(cameraStartPos, cameraStartLookDir, glm::vec3{0.0f, 0.0f, 1.0f});
    CameraManager::setNearPlane(100.0f);
    CameraManager::setFarPlane(50000.0f);

    int viewportW = 1280;
    int viewportH = 720;

    CameraManager::initialize(viewportW, viewportH);
    CameraManager::currentCamera = camera;
    CameraManager::enableCameraLook();
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    activateGLDebugOutput();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);

    int numInstances = 10000;
    float accelerationScalar;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> velocities;
    {
        std::mt19937 re{std::random_device()()};
        float minRadius = 1000.0f;
        float maxRadius = 2000.0f;
        float zOffset = 50.0f;
        std::uniform_real_distribution radiusDist{minRadius, maxRadius};
        std::uniform_real_distribution angleDist{0.0f, glm::radians(360.0f)};
        std::uniform_real_distribution zDist{-zOffset, zOffset};
        float maxSpeed = 200.0f;
        float maxVelocityOffsetRadius = 0.1f;
        accelerationScalar = maxSpeed * maxSpeed * minRadius * 0.05f;
        for (int i = 0; i < numInstances; i++) {
            float r = radiusDist(re);
            float a = angleDist(re);
            positions.emplace_back(r * glm::cos(a), r * glm::sin(a), zDist(re));
            glm::vec3 velocityDir = glm::normalize(glm::cross(positions[i], {0.0f, 0.0f, 1.0f}));
            velocityDir += glm::vec3{glm::rotate(glm::mat4(1.0f), angleDist(re), velocityDir) * glm::vec4{0.0f, 0.0f, maxVelocityOffsetRadius, 0.0f}};
            velocityDir = glm::normalize(velocityDir);
            float speed = glm::sqrt(accelerationScalar / glm::length(positions[i]));
            velocities.emplace_back(speed * velocityDir);
        }
    }

    float sphereScale = 100.0f;
    glm::mat4 sphereModel = glm::scale(glm::mat4{1.0f}, glm::vec3{sphereScale});

    std::array<GLuint, 3> sphereBuffers;
    GLuint& sphereVBO = sphereBuffers[0];
    GLuint& sphereEBO = sphereBuffers[1];
    GLuint& sphereIBO = sphereBuffers[2];
    glCreateBuffers(3, sphereBuffers.data());
    GLuint numSphereIndices;
    {
        MeshData sphereMesh = loadMesh(meshesPath / "sphere.obj");
        numSphereIndices = sphereMesh.indices.size();
        storeVectorGLBuffer(sphereVBO, sphereMesh.vertices);
        storeVectorGLBuffer(sphereEBO, sphereMesh.indices);
    }
    glNamedBufferStorage(sphereIBO, sizeof(glm::vec3) * numInstances, nullptr, GL_DYNAMIC_STORAGE_BIT);

    GLuint sphereVAO;
    glCreateVertexArrays(1, &sphereVAO);
    storeMesh(sphereVAO, sphereVBO, sphereEBO);
    GLuint instanceVAO;
    glCreateVertexArrays(1, &instanceVAO);
    storeMesh(instanceVAO, sphereVBO, sphereEBO);
    glVertexArrayVertexBuffer(instanceVAO, 1, sphereIBO, 0, sizeof(glm::vec3));
    glEnableVertexArrayAttrib(instanceVAO, 5);
    glVertexArrayAttribBinding(instanceVAO, 5, 1);
    glVertexArrayAttribFormat(instanceVAO, 5, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayBindingDivisor(instanceVAO, 1, 1);

    GLuint diffuseShaderProgram;
    GLuint diffuseInstancedShaderProgram;
    {
        GLuint diffuseVertexShader = createShaderSPIRV(GL_VERTEX_SHADER, shaderBinPath / "diffuse.vert.spv");
        GLuint diffuseInstancedVertexShader = createShaderSPIRV(GL_VERTEX_SHADER, shaderBinPath / "diffuse_instanced.vert.spv");
        GLuint diffuseFragmentShader = createShaderSPIRV(GL_FRAGMENT_SHADER, shaderBinPath / "diffuse.frag.spv");
        diffuseShaderProgram = createProgram({diffuseVertexShader, diffuseFragmentShader});
        diffuseInstancedShaderProgram = createProgram({diffuseInstancedVertexShader, diffuseFragmentShader});
        glDeleteShader(diffuseVertexShader);
        glDeleteShader(diffuseInstancedVertexShader);
        glDeleteShader(diffuseFragmentShader);
    }

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

        for (int i = 0; i < numInstances; i++) {
            velocities[i] += (deltaTime * accelerationScalar / glm::pow(glm::dot(positions[i], positions[i]), 1.5f)) * -positions[i];
            positions[i] += deltaTime * velocities[i];
        }

        glNamedBufferSubData(sphereIBO, 0, sizeof(glm::vec3) * numInstances, positions.data());

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = CameraManager::getViewMatrix();
        glm::mat4 projection = CameraManager::getProjectionMatrix();

        glProgramUniformMatrix4fv(diffuseShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projection));
        glProgramUniformMatrix4fv(diffuseShaderProgram, 1, 1, GL_FALSE, glm::value_ptr(view));
        glProgramUniformMatrix4fv(diffuseShaderProgram, 2, 1, GL_FALSE, glm::value_ptr(sphereModel));
        glProgramUniform3fv(diffuseShaderProgram, 3, 1, glm::value_ptr(glm::vec3{1.0f, 1.0f, 0.0f}));

        glProgramUniformMatrix4fv(diffuseInstancedShaderProgram, 0, 1, GL_FALSE, glm::value_ptr(projection * view));
        glProgramUniform3fv(diffuseInstancedShaderProgram, 3, 1, glm::value_ptr(glm::vec3{1.0f, 1.0f, 1.0f}));

        glUseProgram(diffuseShaderProgram);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, numSphereIndices, GL_UNSIGNED_INT, nullptr);
        glUseProgram(diffuseInstancedShaderProgram);
        glBindVertexArray(instanceVAO);
        glDrawElementsInstanced(GL_TRIANGLES, numSphereIndices, GL_UNSIGNED_INT, nullptr, numInstances);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(diffuseShaderProgram);
    glDeleteProgram(diffuseInstancedShaderProgram);
    glDeleteVertexArrays(1, &instanceVAO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereIBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteBuffers(1, &sphereVBO);
    CameraManager::terminate();
}
