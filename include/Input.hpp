#pragma once

#include "GLFWHandle.hpp"

#include <variant>
#include <tuple>

class GLFWwindow;

enum class Key {
    A,
    D,
    F,
    S,
    V,
    W,
    Escape,
};

enum class KeyState {
    Press,
    Release,
};

enum class MouseButton {
    Left,
};

enum class MouseButtonState {
    Press,
    Release,
};

class InputManager {
public:
    InputManager(GLFWwindow* window);
    ~InputManager() = default;

    InputManager(InputManager const&) = delete;
    InputManager(InputManager&&) = delete;

    KeyState getKey(Key key) const;

    MouseButtonState getMouseButton(MouseButton button) const;

    std::tuple<double, double> getCursorPos() const;
    void setCursorPos(double x, double y);

    void enableCursor();
    void disableCursor();

    void pollEvents();

private:
    GLFWHandle glfw_handle;
    GLFWwindow* glfw_window;
};
