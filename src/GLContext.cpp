#include "GLContext.hpp"

#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <cassert>
#include <iostream>
#include <stdexcept>

GLContext* GLContext::current_context = nullptr;

GLContext::GLContext(GLFWwindow* window): glfw_window(window) {
    assert(this->glfw_window);
    GLContext* const prev_context = GLContext::current_context;
    this->makeCurrent();
    if (!gladLoadGLContext(&this->gl, glfwGetProcAddress)) {
        if (prev_context) {
            prev_context->makeCurrent();
        }
        throw std::runtime_error("Failed to initialize GL context");
    }
}

GLContext::~GLContext() {
    if (current_context == this) {
        current_context = nullptr;
        glfwMakeContextCurrent(nullptr);
    }
}

void GLContext::makeCurrent() {
    glfwMakeContextCurrent(this->glfw_window);
    GLContext::current_context = this;
}

void GLContext::EnableDebugOutput() {
    gl.Enable(GL_DEBUG_OUTPUT);
}

void GLContext::DisableDebugOutput() {
    gl.Disable(GL_DEBUG_OUTPUT);
}

void GLContext::EnableDebugOutputSynchronous() {
    gl.Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

void GLContext::DisableDebugOutputSynchronous() {
    gl.Disable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

namespace {
void defaultDebugCallback(GLenum, GLenum, GLuint, GLenum severity, GLsizei, GLchar const* const message, void const* const) {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::cout << message << "\n";
    }
}
}  // namespace

void GLContext::DebugMessageCallback() {
    gl.DebugMessageCallback(defaultDebugCallback, nullptr);
}
