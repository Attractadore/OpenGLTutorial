#pragma once

#include "GLFWHandle.hpp"
#include "Window.hpp"

class OpenGLContext {
public:
    OpenGLContext(Window& window);
    ~OpenGLContext();

    OpenGLContext(OpenGLContext const& other) = delete;
    OpenGLContext(OpenGLContext&& other);

    void makeCurrent();

    void enableDebugOutput();
    void disableDebugOutput();

private:
    static OpenGLContext* current_context;

    GLFWHandle glfw_handle;
    Window& window;
};
