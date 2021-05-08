#pragma once

class GLFWHandle {
public:
    GLFWHandle();
    ~GLFWHandle();

    GLFWHandle(const GLFWHandle& other);
    GLFWHandle(GLFWHandle&& other);

private:
    static unsigned counter;
};
