#pragma once

#include "GLFWHandle.hpp"

#include <string>

class GLFWwindow;

class Window {
public:
    enum class GraphicsAPI {
        OpenGL,
    };

    Window(unsigned width, unsigned height, std::string const& title = "", GraphicsAPI api = GraphicsAPI::OpenGL);
    ~Window();

    Window(const Window&) = delete;
    Window(Window&& other) = delete;

    enum class Error {
        None,
        System,
    };

    bool shouldClose();

    Error swapBuffers();

public:
    GraphicsAPI api;

private:
    GLFWHandle glfw_handle;
    GLFWwindow* glfw_window;
};
