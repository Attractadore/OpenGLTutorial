#pragma once

#include <glad/gl.h>

#include <string>
#include <vector>

GLuint createShaderGLSL(GLenum shaderType, const std::string& shaderPath);
GLuint createShaderSPIRV(GLenum shaderType, const std::string& shaderPath, const std::string& entryPoint = "main");
GLuint createProgram(const std::vector<GLuint>& shaders);
