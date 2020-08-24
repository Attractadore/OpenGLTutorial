#pragma once

#include "glad.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <filesystem>
#include <vector>

struct MeshVertex {
    glm::vec3 position;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec3 normal;
    glm::vec2 tex;
};

struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<GLuint> indices;
};

MeshData loadMesh(const std::filesystem::path& scenePath, std::size_t meshIndex = 0);

void storeMesh(GLuint VAO, GLuint VBO, GLuint EBO);
