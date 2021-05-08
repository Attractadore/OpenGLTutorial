#pragma once

#include <glad/gl.h>

#include <array>
#include <string>

GLuint createTexture2D(const std::string& path, GLenum texFormat, GLenum dataFormat, GLenum dataType);
GLuint createTextureCubeMap(const std::array<std::string, 6>& paths, GLenum texFormat, GLenum dataFormat, GLenum dataType);
