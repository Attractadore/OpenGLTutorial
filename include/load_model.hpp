#pragma once

#include "glad.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <string>
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

struct MeshGLRepr {
    GLuint VBO;
    GLuint EBO;
    GLuint VAO;
    GLuint numIndices;
};

MeshData loadMesh(const std::string& path, std::size_t meshIndex = 0);

MeshGLRepr createMeshGLRepr(const std::string& path, std::size_t meshIndex = 0);
void deleteMeshGLRepr(MeshGLRepr&);

void storeMesh(GLuint VAO, GLuint VBO, GLuint EBO);
