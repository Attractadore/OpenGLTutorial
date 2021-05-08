#include "Input.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cassert>

InputManager::InputManager(GLFWwindow* const window) : glfw_window{window} {
    assert(window);
}

void InputManager::pollEvents(){
    glfwPollEvents();
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to poll events");
    }
}

std::tuple<double, double> InputManager::getCursorPos() const{
    double x, y;
    glfwGetCursorPos(this->glfw_window, &x, &y);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to get cursor pos");
    }
    return {x, y};
}

void InputManager::setCursorPos(const double x, const double y) {
    glfwSetCursorPos(this->glfw_window, x, y);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to set cursor pos");
    }
}

namespace {
int getGLFWKey(const Key key) {
    switch(key) {
        case Key::A:
            return GLFW_KEY_A;
        case Key::D:
            return GLFW_KEY_D; 
        case Key::F:
            return GLFW_KEY_F;
        case Key::S:
            return GLFW_KEY_S; 
        case Key::V:
            return GLFW_KEY_V;
        case Key::W:
            return GLFW_KEY_W; 
        case Key::Escape:
            return GLFW_KEY_ESCAPE;
    }

    assert(!"Unhandled enum value");
}

KeyState getFromGLFWKeyState(const int key_state) {
    switch(key_state) {
        case GLFW_PRESS:
            return KeyState::Press;
        case GLFW_RELEASE:
            return KeyState::Release;
    }

    assert(!"Unhandled enum value");
}

int getGLFWMouseButton(const MouseButton button) {
    switch(button) {
        case MouseButton::Left:
            return GLFW_MOUSE_BUTTON_LEFT;
    }

    assert(!"Unhandled enum value");
}

MouseButtonState getFromGLFWMouseButtonState(const int button_state) {
    switch (button_state) {
        case GLFW_PRESS:
            return MouseButtonState::Press;
        case GLFW_RELEASE:
            return MouseButtonState::Release;
    }

    assert(!"Unhandled enum value");
}
}

KeyState InputManager::getKey(const Key key) const{
    const int key_state = glfwGetKey(this->glfw_window, getGLFWKey(key));
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to get key state");
    }
    return getFromGLFWKeyState(key_state);
}

MouseButtonState InputManager::getMouseButton(const MouseButton button) const {
    const int button_state = glfwGetMouseButton(this->glfw_window, getGLFWMouseButton(button));
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to get mouse button state");
    }
    return getFromGLFWMouseButtonState(button_state);
}

void InputManager::enableCursor() {
    glfwSetInputMode(this->glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to set cursor state");
    }
}

void InputManager::disableCursor() {
    glfwSetInputMode(this->glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
        throw std::runtime_error("Failed to set cursor state");
    }
}
