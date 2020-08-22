#include "load_shader.hpp"

#include <fstream>
#include <regex>

std::string loadGLSL(const std::filesystem::path& shaderPath) {
    std::ifstream is(shaderPath, std::ios::binary);
    if (!is) {
        throw std::runtime_error("Failed to load GLSL from " + shaderPath.string());
    }
    auto fileSize = std::filesystem::file_size(shaderPath);
    std::string fileContents(fileSize, ' ');
    is.read(fileContents.data(), fileSize);
    std::regex includeRegex(R"(#include *"(.+)\")");
    std::smatch regex_matches;
    while (std::regex_search(fileContents, regex_matches, includeRegex)) {
        fileContents.replace(regex_matches[0].first, regex_matches[0].second, loadGLSL(shaderPath.parent_path() / regex_matches[1].str()));
    }
    return fileContents;
}

std::vector<char> loadSPIRV(const std::filesystem::path& shaderPath){

}

GLuint createShaderGLSL(GLenum shaderType, const std::filesystem::path& shaderPath) {
    GLuint shader = glCreateShader(shaderType);
        std::string shaderSource = loadGLSL(shaderPath);
        const char* shaderSourceCStr = shaderSource.c_str();
        glShaderSource(shader, 1, &shaderSourceCStr, nullptr);
        glCompileShader(shader);
    return shader;
}

GLuint createShaderSPIRV(GLenum shaderType, const std::filesystem::path& shaderPath, const std::string& entryPoint){
    GLuint shader = glCreateShader(shaderType);
    std::vector<char> shaderBinary = loadSPIRV(shaderPath);
    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderBinary.data(), shaderBinary.size());
    glSpecializeShader(shader, entryPoint.c_str(), 0, nullptr, nullptr);
    return shader;
}

GLuint createProgram(const std::vector<GLuint>& shaders) {
    GLuint program = glCreateProgram();
    for (auto& shader: shaders) {
        glAttachShader(program, shader);
    }
    glLinkProgram(program);
    for (auto& shader: shaders) {
        glDetachShader(program, shader);
    }
    return program;

}
