#pragma once

#include "glad.h"

#include <filesystem>
#include <vector>

void loadTexture2D(GLuint texID, const std::filesystem::path& imagePath, GLenum texFormat, GLenum dataFormat, GLenum dataType);
void loadTextureCubeMap(GLuint texID, const std::vector<std::filesystem::path>& imagePaths, GLenum texFormat, GLenum dataFormat, GLenum dataType);
