#pragma once

#include "glad.h"

#include <filesystem>
#include <vector>

GLuint createShaderGLSL(GLenum shaderType, const std::filesystem::path& shaderPath);
GLuint createShaderSPIRV(GLenum shaderType, const std::filesystem::path& shaderPath, const std::string& entryPoint = "main");
GLuint createProgram(const std::vector<GLuint>& shaders);
