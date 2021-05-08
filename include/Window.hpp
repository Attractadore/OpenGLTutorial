#pragma once

#include "GLFWHandle.hpp"

#include <memory>
#include <string>
#include <tuple>

class GLFWwindow;
class GLContext;
class InputManager;

class Window {
public:
    enum class GraphicsAPI {
        OpenGL,
    };

    Window(unsigned width, unsigned height, std::string const& title = "", GraphicsAPI api = GraphicsAPI::OpenGL);
    ~Window();

    Window(const Window&) = delete;
    Window(Window&& other) = delete;

    bool shouldClose() const;

    void swapBuffers();

    std::tuple<unsigned, unsigned> dimensions() const;
    double aspectRatio() const;

    GLContext* glContext() noexcept;
    GLContext const* glContext() const noexcept;
    InputManager* inputManager() noexcept;
    InputManager const* inputManager() const noexcept;

public:
    const GraphicsAPI api;

private:
    GLFWHandle glfw_handle;
    GLFWwindow* glfw_window;
    std::unique_ptr<GLContext> gl_context;
    std::unique_ptr<InputManager> input_manager;
};
