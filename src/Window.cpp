#include "Window.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace {
Window::Error getGLFWError() {
    const int error = glfwGetError(nullptr);
    switch (error) {
        case GLFW_NO_ERROR:
            return Window::Error::None;
        default:
            return Window::Error::System;
    }
}
}  // namespace

Window::Window(const unsigned width, const unsigned height, std::string const& title) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    this->glfw_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
    if (!this->glfw_window) {
        throw std::runtime_error("Failed to create Window");
    }
}

Window::~Window() {
    glfwDestroyWindow(this->glfw_window);
}

Window::Error Window::makeContextCurrent() {
    glfwMakeContextCurrent(this->glfw_window);
    return getGLFWError();
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(this->glfw_window);
}

Window::Error Window::swapBuffers() {
    glfwSwapBuffers(this->glfw_window);
    return getGLFWError();
}
