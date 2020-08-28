#include "load_model.hpp"
#include "util.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include <string>

MeshData loadMesh(const std::filesystem::path& scenePath, std::size_t meshIndex) {
    if (!std::filesystem::exists(scenePath)) {
        throw std::runtime_error("Failed to load scene from " + scenePath.string());
    }
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(scenePath.c_str(),
                                             aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
    if (!scene) {
        throw std::runtime_error("Error while loading scene from " + scenePath.string());
    }
    if (meshIndex >= scene->mNumMeshes) {
        throw std::runtime_error("No mesh numbered " + std::to_string(meshIndex) + " in scene " + scenePath.string());
    }

    MeshData newMeshData;
    auto mesh = scene->mMeshes[meshIndex];
    for (std::size_t j = 0; j < mesh->mNumVertices; j++) {
        MeshVertex vert;
        vert.position = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};
        vert.tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z};
        vert.bitangent = {mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z};
        vert.normal = {mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
        vert.tex = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
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

MeshGLRepr createMeshGLRepr(const std::filesystem::path& scenePath, std::size_t meshIndex) {
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
    {
        constexpr std::array<GLuint, 5> numComponents = {3, 3, 3, 3, 2};
        GLuint offset = 0;
        for (int i = 0; i < 5; i++) {
            glEnableVertexArrayAttrib(VAO, i);
            glVertexArrayAttribBinding(VAO, i, 0);
            glVertexArrayAttribFormat(VAO, i, numComponents[i], GL_FLOAT, GL_FALSE, offset);
            offset += numComponents[i] * sizeof(GLfloat);
        }
    }
}
