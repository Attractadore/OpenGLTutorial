#include "glad.h"

#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <iostream>

void windowResizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

int main(){

    int defaultWindowWidth = 800;
    int defaultWindowHeight = 600;
    std::string windowTitle = "OpenGL Tutorial";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint triangleShaderProgram;
    GLuint vertexVBO;
    GLuint vertexEBO;
    GLuint triangleVAO;

    std::vector<GLfloat> vertexCoords =
    {
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
    };

    std::vector<GLuint> vertexIndices =
    {
        0, 1, 2,
        2, 3, 0
    };

    const char* vertexShaderSource =
         R"(#version 330 core
            layout (location = 0) in vec3 aPos;

            void main()
            {
               gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
            })";

    const char* fragmentShaderSource =
         R"(#version 330 core
            out vec4 FragColor;

            void main()
            {
               FragColor = vec4(1.0, 0.5, 0.0, 1.0);
            })";

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(defaultWindowWidth, defaultWindowHeight, windowTitle.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    triangleShaderProgram = glCreateProgram();
    glAttachShader(triangleShaderProgram, vertexShader);
    glAttachShader(triangleShaderProgram, fragmentShader);
    glLinkProgram(triangleShaderProgram);
    glDetachShader(triangleShaderProgram, vertexShader);
    glDetachShader(triangleShaderProgram, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &vertexVBO);
    glGenBuffers(1, &vertexEBO);

    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof (GLfloat) * vertexCoords.size(), vertexCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
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
        glBindVertexArray(triangleVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
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
