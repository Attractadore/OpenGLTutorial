#include "Window.hpp"
#include "Input.hpp"
#include "GLContext.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>

Window::Window(const unsigned width, const unsigned height, std::string const& title, const GraphicsAPI api) : api{api} {
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
    this->input_manager = std::make_unique<InputManager>(this->glfw_window);
    if (api == GraphicsAPI::OpenGL) {
        this->gl_context = std::make_unique<GLContext>(this->glfw_window);
    }
}

Window::~Window() {
    glfwDestroyWindow(this->glfw_window);
}

bool Window::shouldClose() const{
    return glfwWindowShouldClose(this->glfw_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(this->glfw_window);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to swap Window buffers");
    }
}

std::tuple<unsigned, unsigned> Window::dimensions() const {
    int width, height;
    glfwGetWindowSize(this->glfw_window, &width, &height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to get Window dimensions");
    }
    return {width, height};
}

double Window::aspectRatio() const {
    auto [w, h] = this->dimensions();
    return static_cast<double>(w) / static_cast<double>(h); 
}

GLContext* Window::glContext() noexcept{
    return this->gl_context.get();
}

GLContext const* Window::glContext() const noexcept{
    return this->gl_context.get();
}

InputManager* Window::inputManager() noexcept{
    return this->input_manager.get();
}

InputManager const* Window::inputManager() const noexcept {
    return this->input_manager.get();
}
