#include "load_shader.hpp"

#include <fstream>

std::string loadGLSL(const std::filesystem::path& shaderPath) {
    std::ifstream is(shaderPath, std::ios::binary);
    if (!is) {
        throw std::runtime_error("Failed to load GLSL from " + shaderPath.string());
    }
    auto fileSize = std::filesystem::file_size(shaderPath);
    std::string fileContents(fileSize, ' ');
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

std::vector<char> loadSPIRV(const std::filesystem::path& shaderPath) {
    std::ifstream is(shaderPath, std::ios::binary);
    if (!is) {
        throw std::runtime_error("Failed to load SPIR-V from " + shaderPath.string());
    }
    auto fileSize = std::filesystem::file_size(shaderPath);
    std::vector<char> fileContents(fileSize);
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

namespace {
bool BShaderCompiled(GLuint shader) {
    GLint bCompileSuccess = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &bCompileSuccess);
    return bCompileSuccess == GL_TRUE;
}

std::string GetShaderCompileLog(GLuint shader) {
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    logLength = std::max(1, logLength) - 1;
    std::string log(logLength, ' ');
    GLsizei usedLogLength = logLength;
    glGetShaderInfoLog(shader, logLength + 1, &usedLogLength, log.data());
    log.resize(usedLogLength);
    return log;
}
}  // namespace

GLuint createShaderGLSL(GLenum shaderType, const std::filesystem::path& shaderPath) {
    GLuint shader = glCreateShader(shaderType);
    std::string shaderSource = loadGLSL(shaderPath);
    const char* shaderSourceCStr = shaderSource.c_str();

    glShaderSource(shader, 1, &shaderSourceCStr, nullptr);
    glCompileShader(shader);

    if (!BShaderCompiled(shader)) {
        std::string compileLog = GetShaderCompileLog(shader);
        glDeleteShader(shader);
        throw std::runtime_error("Failed to compile GLSL shader:\n" + compileLog);
    }

    return shader;
}

GLuint createShaderSPIRV(GLenum shaderType, const std::filesystem::path& shaderPath, const std::string& entryPoint) {
    GLuint shader = glCreateShader(shaderType);
    std::vector<char> shaderBinary = loadSPIRV(shaderPath);

    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderBinary.data(), shaderBinary.size());
    glSpecializeShader(shader, entryPoint.c_str(), 0, nullptr, nullptr);

    if (!BShaderCompiled(shader)) {
        glDeleteShader(shader);
        throw std::runtime_error("Failed to specialize SPIR-V shader");
    }

    return shader;
}

namespace {
bool BProgramLinked(GLuint program) {
    GLint bLinkSuccess = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &bLinkSuccess);
    return bLinkSuccess == GL_TRUE;
}

std::string GetProgramLinkLog(GLuint program) {
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    logLength = std::max(1, logLength) - 1;
    std::string log(logLength, ' ');
    GLsizei usedLogLength = logLength;
    glGetProgramInfoLog(program, logLength + 1, &usedLogLength, log.data());
    log.resize(usedLogLength);
    return log;
}
}  // namespace

GLuint createProgram(const std::vector<GLuint>& shaders) {
    GLuint program = glCreateProgram();

    for (auto& shader: shaders) {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    if (!BProgramLinked(program)) {
        std::string log = GetProgramLinkLog(program);
        glDeleteProgram(program);
        throw std::runtime_error("Failed to link shader program: " + log);
    }

    for (auto& shader: shaders) {
        glDetachShader(program, shader);
    }

    return program;
}
