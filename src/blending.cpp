#include "Camera.hpp"
#include "GLContext.hpp"
#include "Window.hpp"
#include "Input.hpp"
#include "load_model.hpp"
#include "load_shader.hpp"
#include "load_texture.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <chrono>

struct CameraLookData {
    glm::dvec2 cursor_pos;
    glm::dvec2 sensitivity;
    glm::bvec2 invert;
    bool enabled;
};

glm::dvec2 cameraGetLookInput(CameraLookData& look_data, InputManager const& input) {
    auto [new_cursor_x, new_cursor_y] = input.getCursorPos();

    glm::dvec2 new_cursor_pos{new_cursor_x, new_cursor_y};
    glm::dvec2 delta_cursor_pos = new_cursor_pos - look_data.cursor_pos;
    look_data.cursor_pos = new_cursor_pos;

    return delta_cursor_pos;
}

void cameraApplyLookInput(Camera& camera, CameraLookData const& look_data, glm::dvec2 look_input) {
    if (!look_data.enabled) {
        return;
    }

    if (!look_data.invert.x) {
        look_input.x = -look_input.x;
    }
    if (!look_data.invert.y) {
        look_input.y = -look_input.y;
    }

    look_input *= look_data.sensitivity;

    camera.addYaw(look_input.x);
    camera.addPitch(look_input.y);
}

void cameraLookDisable(CameraLookData& look_data, InputManager& input) {
    if (look_data.enabled) {
        input.enableCursor();
        look_data.enabled = false;
    }
}

void cameraLookEnable(CameraLookData& look_data, InputManager& input) {
    if (!look_data.enabled) {
        input.disableCursor();
        std::tie(look_data.cursor_pos.x, look_data.cursor_pos.y) = input.getCursorPos();
        look_data.enabled = true;
    }
}

void cameraLookSetState(CameraLookData& look_data, InputManager& input) {
    if (input.getKey(Key::Escape) == KeyState::Press) {
        cameraLookDisable(look_data, input);
    }
    if (input.getMouseButton(MouseButton::Left) == MouseButtonState::Press) {
        cameraLookEnable(look_data, input);
    }
}

glm::mat4 getCameraViewMatrix(Camera const& camera) {
    auto cameraPos = camera.cameraPos;
    auto cameraForwardVector = camera.getCameraForwardVector();
    auto cameraUpVector = camera.getCameraUpVector();
    return glm::lookAt(cameraPos, cameraPos + cameraForwardVector, cameraUpVector);
}

struct ProjectionData {
    double fov, near, far;
};

glm::mat4 getProjectionMatrix(Window const& window, ProjectionData const& proj_data) {
    return glm::perspective(proj_data.fov, window.aspectRatio(), proj_data.near, proj_data.far);
}

glm::ivec3 cameraGetMovementInput(InputManager const& input) {
    glm::ivec3 movement_input{0};
    if (input.getKey(Key::W) == KeyState::Press) {
        movement_input.x++;
    }
    if (input.getKey(Key::S) == KeyState::Press) {
        movement_input.x--;
    }
    if (input.getKey(Key::D) == KeyState::Press) {
        movement_input.y++;
    }
    if (input.getKey(Key::A) == KeyState::Press) {
        movement_input.y--;
    }
    if (input.getKey(Key::F) == KeyState::Press) {
        movement_input.z++;
    }
    if (input.getKey(Key::V) == KeyState::Press) {
        movement_input.z--;
    }
    return movement_input;
}

void cameraApplyMovementInput(Camera& camera, const double camera_speed, const double delta_time, const glm::ivec3 movement_input) {
    const glm::vec3 float_movement_input = movement_input;
    glm::vec3 dir_vec = float_movement_input.x * camera.getCameraForwardVector() +
                        float_movement_input.y * camera.getCameraRightVector() +
                        float_movement_input.z * camera.getCameraUpVector();
    if (glm::dot(dir_vec, dir_vec) > 0.0f) {
        camera.cameraPos += glm::normalize(dir_vec) * static_cast<float>(camera_speed * delta_time);
    }
}

int main() {
    const std::string assetsPath = "assets";
    const std::string meshesPath = assetsPath + "/meshes";
    const std::string texturePath = assetsPath + "/textures/blending";
    const std::string shaderBinPath = assetsPath + "/shaders/bin/blending";

    int viewportW = 1280;
    int viewportH = 720;

    Window window(viewportW, viewportH);
    if (!window.glContext()) {
        throw std::runtime_error("GL was not loaded");
    }
    GLContext& gl = *window.glContext();
    if (!window.inputManager()) {
        throw std::runtime_error("Input was not loaded");
    }
    InputManager& input = *window.inputManager();
    gl.EnableDebugOutput();
    gl.EnableDebugOutputSynchronous();
    gl.DebugMessageCallback();

    const double camera_speed = 5.0;
    glm::vec3 cameraStartPos = {0.0f, -10.0f, 4.0f};
    glm::vec3 cameraStartLookDir = -cameraStartPos;
    Camera camera(cameraStartPos, cameraStartLookDir, glm::vec3{0.0f, 0.0f, 1.0f});
    CameraLookData camera_look_data = {
        .sensitivity{0.1, 0.1}
    };
    cameraLookEnable(camera_look_data, input);
    // GLFW seems to center the cursor after the first poll
    // if it is disabled somewhere in the beginning, which leads to 
    // unpredictable camera repositioning.
    // This is a hack to take care of that.
    camera_look_data.cursor_pos = glm::dvec2{viewportW, viewportH} / 2.0;

    ProjectionData proj_data = {
        .fov = glm::radians(45.0),
        .near = 0.1,
        .far = 30.0,
    };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    MeshGLRepr cubeMesh = createMeshGLRepr(meshesPath + "/cube.obj");
    MeshGLRepr planeMesh = createMeshGLRepr(meshesPath + "/transparentplane.obj");
    MeshGLRepr groundMesh = createMeshGLRepr(meshesPath + "/circularplane.obj");

    GLuint diffuseTexture = createTexture2D(texturePath + "/../container2.png", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
    GLuint windowTexture = createTexture2D(texturePath + "/window.png", GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);

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
        GLuint diffuseVertexShader = createShaderSPIRV(GL_VERTEX_SHADER, shaderBinPath + "/diffuse.vert.spv");
        GLuint diffuseFragmentShader = createShaderSPIRV(GL_FRAGMENT_SHADER, shaderBinPath + "/diffuse.frag.spv");
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
        glm::vec3 rhsDist = rhs - camera.cameraPos;
        glm::vec3 lhsDist = lhs - camera.cameraPos;
        return glm::dot(rhsDist, rhsDist) > glm::dot(lhsDist, lhsDist);
    };

    std::vector<glm::vec3> cubePositions = {
        {5.0f, 3.0f, 0.0f},
        {-5.0f, 3.0f, 0.0f},
        {5.0f, -3.0f, 0.0f},
        {-5.0f, -3.0f, 0.0f}};

    auto previous_time = std::chrono::steady_clock::now();
    while (!window.shouldClose()) {
        const auto current_time = std::chrono::steady_clock::now();
        const double delta_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                current_time - previous_time
        ).count() / 1e9;

        input.pollEvents();

        cameraLookSetState(camera_look_data, input);
        if (camera_look_data.enabled) {
            glm::dvec2 camera_input = cameraGetLookInput(camera_look_data, input);
            cameraApplyLookInput(camera, camera_look_data, camera_input);
        }

        {
            glm::ivec3 camera_input = cameraGetMovementInput(input);
            cameraApplyMovementInput(camera, camera_speed, delta_time, camera_input);
        }

        std::sort(windowPositions.begin(), windowPositions.end(), distSort);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = getCameraViewMatrix(camera);
        glm::mat4 projection = getProjectionMatrix(window, proj_data);

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

        window.swapBuffers();

        previous_time = current_time;
    }

    glDeleteProgram(diffuseShaderProgram);
    glDeleteSamplers(1, &clampSampler);
    glDeleteSamplers(1, &wrapSampler);
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &windowTexture);
    deleteMeshGLRepr(cubeMesh);
    deleteMeshGLRepr(planeMesh);
    deleteMeshGLRepr(groundMesh);
}
