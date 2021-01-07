#pragma once

#include "glad.h"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
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
    glm::mat4 model;
    glm::mat3 normal;
    GLuint VBO;
    GLuint EBO;
    GLuint VAO;
    GLuint numIndices;
    bool bCullFaces : 1;
    bool bCastsShadows : 1;
};

MeshData loadMesh(const std::string& path, std::size_t meshIndex = 0);

MeshGLRepr createMeshGLRepr(const std::string& path, std::size_t meshIndex = 0);
void deleteMeshGLRepr(MeshGLRepr&);

void storeMesh(GLuint VAO, GLuint VBO, GLuint EBO);
