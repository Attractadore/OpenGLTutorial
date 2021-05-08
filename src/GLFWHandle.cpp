#include "GLFWHandle.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>

unsigned GLFWHandle::counter = 0;

GLFWHandle::GLFWHandle() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    GLFWHandle::counter++;
}

GLFWHandle::~GLFWHandle() {
    GLFWHandle::counter--;
    if (!GLFWHandle::counter) {
        glfwTerminate();
    }
}

GLFWHandle::GLFWHandle(const GLFWHandle&) {
    GLFWHandle::counter++;
}

GLFWHandle::GLFWHandle(GLFWHandle&&) {
    GLFWHandle::counter++;
}
