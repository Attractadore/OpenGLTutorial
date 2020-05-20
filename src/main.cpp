#include "glad.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>

void debugMessageCallback(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam){
    if (severity​ != GL_DEBUG_SEVERITY_NOTIFICATION){
        std::cout << message​ << std::endl;
    }
}

std::vector<char> getBinary(std::filesystem::path filePath){
    std::ifstream is(filePath, std::ios::binary);
    if (!is){
        return {};
    }
    auto fileSize = std::filesystem::file_size(filePath);
    std::vector<char> fileContents(fileSize);
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

void windowResizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

int main(){

    int defaultWindowWidth = 800;
    int defaultWindowHeight = 600;
    std::string windowTitle = "OpenGL Tutorial";

    GLuint vertShader;
    GLuint fragShader;
    GLuint triangleShaderProgram;
    GLuint vertexVBO;
    GLuint vertexEBO;
    GLuint triangleVAO;

    std::vector<GLfloat> vertexCoords =
    {
          0.0f,          1.0f,         1.0f, 0.0f, 0.0f,
          sinf(M_PI/3), -cosf(M_PI/3), 0.0f, 1.0f, 0.0f,
         -sinf(M_PI/3), -cosf(M_PI/3), 0.0f, 0.0f, 1.0f,
    };

    std::vector<GLuint> vertexIndices =
    {
        0, 1, 2,
    };

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(defaultWindowWidth, defaultWindowHeight, windowTitle.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    if (glIsEnabled(GL_DEBUG_OUTPUT)){
        glDebugMessageCallback(debugMessageCallback, nullptr);
    }

    vertShader = glCreateShader(GL_VERTEX_SHADER);
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    auto vertShaderBinary = getBinary("assets/shaders/bin/triangle.vert");
    auto fragShaderBinary = getBinary("assets/shaders/bin/triangle.frag");

    glShaderBinary(1, &vertShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertShaderBinary.data(), vertShaderBinary.size());
    glSpecializeShader(vertShader, "main", 0, nullptr, nullptr);
    glShaderBinary(1, &fragShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragShaderBinary.data(), fragShaderBinary.size());
    glSpecializeShader(fragShader, "main", 0, nullptr, nullptr);
    triangleShaderProgram = glCreateProgram();
    glAttachShader(triangleShaderProgram, vertShader);
    glAttachShader(triangleShaderProgram, fragShader);
    glLinkProgram(triangleShaderProgram);
    glDetachShader(triangleShaderProgram, vertShader);
    glDetachShader(triangleShaderProgram, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    glCreateVertexArrays(1, &triangleVAO);
    glCreateBuffers(1, &vertexVBO);
    glCreateBuffers(1, &vertexEBO);

    glNamedBufferData(vertexVBO, sizeof (GLfloat) * vertexCoords.size(), vertexCoords.data(), GL_STATIC_DRAW);
    glNamedBufferData(vertexEBO, sizeof (GLuint) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);
    glVertexArrayAttribFormat(triangleVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(triangleVAO, 1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat));
    glVertexArrayAttribBinding(triangleVAO, 0, 0);
    glVertexArrayAttribBinding(triangleVAO, 1, 0);
    glVertexArrayVertexBuffer(triangleVAO, 0, vertexVBO, 0, 5 * sizeof (GLfloat));
    glVertexArrayElementBuffer(triangleVAO, vertexEBO);
    glEnableVertexArrayAttrib(triangleVAO, 0);
    glEnableVertexArrayAttrib(triangleVAO, 1);

    while (not glfwWindowShouldClose(window)){

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glClearColor(0.1f, 0.3f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(triangleShaderProgram);
        glUniform1f(0, glfwGetTime());
        glBindVertexArray(triangleVAO);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &vertexVBO);
    glDeleteBuffers(1, &vertexEBO);
    glDeleteProgram(triangleShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
