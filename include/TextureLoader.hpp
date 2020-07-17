#pragma once
#include "glad.h"

#include <boost/functional/hash.hpp>

#include <filesystem>
#include <unordered_map>

class TextureLoader {
public:
    TextureLoader() = delete;

    static GLuint getTextureId2D(const std::string& textureName);
    static GLuint getTextureIdCubeMap(const std::vector<std::string>& textureNames);
    static void freeTexture2D(const std::string& textureName);
    static void freeTextureCubeMap(const std::vector<std::string>& textureNames);
    static void freeTextures();
    static void setTextureRoot(const std::filesystem::path& newTextureRoot);

private:
    static void loadTexture2D(const std::string& textureKey);
    static void loadTextureCubeMap(const std::vector<std::string>& textureKey);

    static std::unordered_map<std::string, GLuint> texture2DMap;
    static std::unordered_map<std::vector<std::string>, GLuint, boost::hash<std::vector<std::string>>> textureCubeMapMap;
    static std::filesystem::path textureRoot;
};
