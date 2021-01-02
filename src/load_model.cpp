#include "load_model.hpp"
#include "util.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <stdexcept>

glm::vec2 AVecToGVec(aiVector2D const& v) {
    return {v.x, v.y};
}

glm::vec3 AVecToGVec(aiVector3D const& v) {
    return {v.x, v.y, v.z};
}

MeshData loadMesh(const std::string& path, std::size_t meshIndex) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
    if (!scene) {
        throw std::runtime_error("Failed to load scene from " + path + ":\n" + importer.GetErrorString());
    }
    if (meshIndex >= scene->mNumMeshes) {
        throw std::runtime_error("No mesh numbered " + std::to_string(meshIndex) + " in scene " + path);
    }

    MeshData newMeshData;
    auto mesh = scene->mMeshes[meshIndex];
    for (std::size_t j = 0; j < mesh->mNumVertices; j++) {
        MeshVertex vert;
        if (mesh->mVertices) {
            vert.position = AVecToGVec(mesh->mVertices[j]);
        }
        if (mesh->mTangents) {
            vert.tangent = AVecToGVec(mesh->mTangents[j]);
        }
        if (mesh->mBitangents) {
            vert.bitangent = AVecToGVec(mesh->mBitangents[j]);
        }
        if (mesh->mNormals) {
            vert.normal = AVecToGVec(mesh->mNormals[j]);
        }
        if (mesh->mTextureCoords[0]) {
            vert.tex = AVecToGVec(mesh->mTextureCoords[0][j]);
        }
        newMeshData.vertices.push_back(std::move(vert));
    }
    for (std::size_t j = 0; j < mesh->mNumFaces; j++) {
        auto face = mesh->mFaces[j];
        for (std::size_t k = 0; k < face.mNumIndices; k++) {
            newMeshData.indices.push_back(face.mIndices[k]);
        }
    }

    return newMeshData;
}

MeshGLRepr createMeshGLRepr(const std::string& scenePath, std::size_t meshIndex) {
    MeshGLRepr repr;
    glCreateBuffers(1, &repr.VBO);
    glCreateBuffers(1, &repr.EBO);
    glCreateVertexArrays(1, &repr.VAO);
    MeshData mesh = loadMesh(scenePath, meshIndex);
    repr.numIndices = mesh.indices.size();
    storeVectorGLBuffer(repr.VBO, mesh.vertices);
    storeVectorGLBuffer(repr.EBO, mesh.indices);
    storeMesh(repr.VAO, repr.VBO, repr.EBO);
    return repr;
}

void deleteMeshGLRepr(MeshGLRepr& repr) {
    glDeleteBuffers(1, &repr.VBO);
    glDeleteBuffers(1, &repr.EBO);
    glDeleteVertexArrays(1, &repr.VAO);
    repr = {0, 0, 0, 0};
}

void storeMesh(GLuint VAO, GLuint VBO, GLuint EBO) {
    glVertexArrayElementBuffer(VAO, EBO);
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(MeshVertex));
    constexpr std::array<GLuint, 5> numComponents = {3, 3, 3, 3, 2};
    GLuint offset = 0;
    for (std::size_t i = 0; i < 5; i++) {
        glEnableVertexArrayAttrib(VAO, i);
        glVertexArrayAttribBinding(VAO, i, 0);
        glVertexArrayAttribFormat(VAO, i, numComponents[i], GL_FLOAT, GL_FALSE, offset);
        offset += numComponents[i] * sizeof(GLfloat);
    }
}
