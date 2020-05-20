#include "glad.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>

void windowResizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void debugFunction(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam​){
    if (severity​ != GL_DEBUG_SEVERITY_NOTIFICATION){
        std::cout << message​ << std::endl;
    }
}

std::string loadShaderSource(std::filesystem::path filePath){
    std::fstream is(filePath);
    if (!is){
        return "";
    }
    auto fileSize = std::filesystem::file_size(filePath);
    std::string fileContents(fileSize, ' ');
    is.read(fileContents.data(), fileSize);
    return fileContents;
}

int main(){

    int defaultWindowWidth = 800;
    int defaultWindowHeight = 800;
    std::string windowTitle = "OpenGL Tutorial";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint triangleShaderProgram;
    GLuint vertexVBO;
    GLuint vertexEBO;
    GLuint triangleVAO;
    GLuint timeUniformLocation;

    std::vector<GLfloat> vertexData =
    {
          0.0f,          1.0f,         1.0f, 0.0f, 0.0f,
          sinf(M_PI/3), -cosf(M_PI/3), 0.0f, 1.0f, 0.0f,
         -sinf(M_PI/3), -cosf(M_PI/3), 0.0f, 0.0f, 1.0f
    };

    std::vector<GLuint> vertexIndices =
    {
        0, 1, 2,
    };

    auto vertexShaderSource = loadShaderSource("assets/shaders/triangle.vert");
    auto fragmentShaderSource = loadShaderSource("assets/shaders/triangle.frag");
    auto vertexShaderSourceCStr = vertexShaderSource.c_str();
    auto fragmentShaderSourceCStr = fragmentShaderSource.c_str();


    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(defaultWindowWidth, defaultWindowHeight, windowTitle.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    if (glIsEnabled(GL_DEBUG_OUTPUT)){
        glDebugMessageCallback(debugFunction, nullptr);
    }

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSourceCStr, nullptr);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &fragmentShaderSourceCStr, nullptr);
    glCompileShader(fragmentShader);
    triangleShaderProgram = glCreateProgram();
    glAttachShader(triangleShaderProgram, vertexShader);
    glAttachShader(triangleShaderProgram, fragmentShader);
    glLinkProgram(triangleShaderProgram);
    glDetachShader(triangleShaderProgram, vertexShader);
    glDetachShader(triangleShaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    timeUniformLocation = glGetUniformLocation(triangleShaderProgram, "time");

    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &vertexVBO);
    glGenBuffers(1, &vertexEBO);

    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof (GLfloat) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void *) (2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

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
        glUniform1f(timeUniformLocation, glfwGetTime());
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
