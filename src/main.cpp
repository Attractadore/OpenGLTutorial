#include "glad.h"

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <IL/il.h>

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

void loadTextureData(GLuint textureId, std::filesystem::path filePath){
    if (!std::filesystem::exists(filePath)){
        return;
    }
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    ILuint loadedImage;
    ilGenImages(1, &loadedImage);
    ilBindImage(loadedImage);
    ilLoadImage(filePath.c_str());
    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    std::vector<std::byte> imageData(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, imageData.data());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    ilDeleteImage(loadedImage);
}

int main(){
    std::string windowTitle = "OpenGL Tutorial";
    float horizontalFOV = 90.0f;
    float aspectRatio = 16.0f / 9.0f;
    int defaultWindowHeight = 720;
    int defaultWindowWidth = defaultWindowHeight * aspectRatio;
    float rotateSpeed = 180.0f;

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint triangleShaderProgram;
    GLuint vertexVBO;
    GLuint vertexEBO;
    GLuint triangleVAO;
    GLuint timeUniformLocation;
    GLuint backgroundTextureUniformLocation;
    GLuint foregroundTextureUniformLocation;
    GLuint modelMatrixUniformLocation;
    GLuint viewMatrixUniformLocation;
    GLuint projectionMatrixUniformLocation;
    GLuint triangleBackgroundTexture;
    GLuint triangleForegroundTexture;

    std::vector<GLfloat> vertexData =
    {
         1.0f,  1.0f,  1.0f, 1.0f, 0.f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f, 1.f, 0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 0.0f, 0.f, 1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, 1.0f, 1.f, 1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, -1.0f, 0.0f, 0.f, 1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f, 1.f, 1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f, 1.0f, 0.f, 0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 0.0f, 1.f, 0.0f, 1.0f, 1.0f,
    };

    std::vector<GLuint> vertexIndices =
    {
        0, 1, 2,
        2, 3, 0,
        0, 1, 4,
        1, 4, 5,
        1, 2, 6,
        1, 5, 6,
        2, 3, 7,
        2, 6, 7,
        0, 3, 7,
        0, 4, 7,
        4, 5, 6,
        4, 7, 6,
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(defaultWindowWidth, defaultWindowHeight, windowTitle.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    if (glIsEnabled(GL_DEBUG_OUTPUT)){
        glDebugMessageCallback(debugFunction, nullptr);
    }
    glEnable(GL_DEPTH_TEST);

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
    backgroundTextureUniformLocation = glGetUniformLocation(triangleShaderProgram, "backTex");
    foregroundTextureUniformLocation = glGetUniformLocation(triangleShaderProgram, "foreTex");
    modelMatrixUniformLocation = glGetUniformLocation(triangleShaderProgram, "model");
    viewMatrixUniformLocation = glGetUniformLocation(triangleShaderProgram, "view");
    projectionMatrixUniformLocation = glGetUniformLocation(triangleShaderProgram, "projection");

    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &vertexVBO);
    glGenBuffers(1, &vertexEBO);
    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof (GLfloat) * vertexData.size(), vertexData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void *) (3 * sizeof(GLfloat)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void *) (6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * vertexIndices.size(), vertexIndices.data(), GL_STATIC_DRAW);

    glGenTextures(1, &triangleBackgroundTexture);
    glGenTextures(1, &triangleForegroundTexture);
    loadTextureData(triangleBackgroundTexture, "assets/textures/tex1.png");
    loadTextureData(triangleForegroundTexture, "assets/textures/tex2.png");

    glm::mat4 view = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -5.0f,});
    glm::mat4 projection = glm::perspective(glm::radians(horizontalFOV / aspectRatio), aspectRatio, 0.1f, 100.0f);

    float previousTime = 0.0f;

    while (not glfwWindowShouldClose(window)){

        float currentTime = glfwGetTime();
        float deltaTime = previousTime - currentTime;

        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(currentTime * rotateSpeed), {0.0f, 1.0f, 0.0f});

        glClearColor(0.1f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(triangleShaderProgram);
        glUniform1f(timeUniformLocation, glfwGetTime());
        glUniform1i(backgroundTextureUniformLocation, 0);
        glUniform1i(foregroundTextureUniformLocation, 1);
        glUniformMatrix4fv(modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(projection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, triangleBackgroundTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, triangleForegroundTexture);

        glBindVertexArray(triangleVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();

        previousTime = currentTime;
    }

    glDeleteTextures(1, &triangleBackgroundTexture);
    glDeleteTextures(1, &triangleForegroundTexture);
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &vertexVBO);
    glDeleteBuffers(1, &vertexEBO);
    glDeleteProgram(triangleShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
