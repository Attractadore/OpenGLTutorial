#include "OpenGLContext.hpp"

#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>

OpenGLContext* OpenGLContext::current_context = nullptr;

OpenGLContext::OpenGLContext(Window& window) : window(window) {
}

OpenGLContext::~OpenGLContext() {
}

OpenGLContext::OpenGLContext(OpenGLContext&& other) : window(other.window) {
}

namespace {
void debugCallback(GLenum, GLenum, GLuint, GLenum severity, GLsizei, GLchar const* const message, void const* const) {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::cout << message << "\n";
    }
}
}

void OpenGLContext::enableDebugOutput() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, nullptr);
}

void OpenGLContext::disableDebugOutput() {
    glDisable(GL_DEBUG_OUTPUT);
}
