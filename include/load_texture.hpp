#pragma once

#include "glad.h"

#include <array>
#include <filesystem>

GLuint createTexture2D(const std::filesystem::path& path, GLenum texFormat, GLenum dataFormat, GLenum dataType);
GLuint createTextureCubeMap(const std::array<std::filesystem::path, 6>& paths, GLenum texFormat, GLenum dataFormat, GLenum dataType);
