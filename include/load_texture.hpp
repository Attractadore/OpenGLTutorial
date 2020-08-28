#pragma once

#include "glad.h"

#include <filesystem>
#include <vector>

GLuint createTexture2D(const std::filesystem::path& imagePath, GLenum texFormat, GLenum dataFormat, GLenum dataType);
GLuint createTextureCubeMap(const std::vector<std::filesystem::path>& imagePaths, GLenum texFormat, GLenum dataFormat, GLenum dataType);
