#pragma once
#include "glad.h"

#include <iostream>

void debugFunction(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::cout << message << std::endl;
    }
}


