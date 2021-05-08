#pragma once

#include "GLFWHandle.hpp"

#include <glad/gl.h>

#include <memory>

class GLFWwindow;

class GLContext {
public:
    GLContext(GLFWwindow* window);
    ~GLContext();

    GLContext(GLContext const& other) = delete;
    GLContext(GLContext&& other) = delete;

    void makeCurrent();

    void EnableDebugOutput();
    void DisableDebugOutput();
    void DebugMessageCallback();

    void EnableDebugOutputSynchronous();
    void DisableDebugOutputSynchronous();

private:
    static GLContext* current_context;

    GladGLContext gl;

    GLFWwindow* glfw_window;
    GLFWHandle glfw_handle;
};
