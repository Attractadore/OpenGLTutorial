#pragma once
#include "glad.h"

#include <boost/functional/hash.hpp>

#include <filesystem>
#include <unordered_map>

class TextureLoader {
public:
    TextureLoader() = delete;

    static GLuint getTextureId2D(const std::string& textureName, bool bSRGBA = true);
    static GLuint getTextureIdCubeMap(const std::vector<std::string>& textureNames, bool bSRGBA = true);
    static void freeTexture2D(const std::string& textureName);
    static void freeTextureCubeMap(const std::vector<std::string>& textureNames);
    static void freeTextures();
    static void setTextureRoot(const std::filesystem::path& newTextureRoot);

private:
    static void loadTexture2D(const std::string& textureKey, bool bSRGBA);
    static void loadTextureCubeMap(const std::vector<std::string>& textureKey, bool bSRGBA);

    static std::unordered_map<std::string, GLuint, boost::hash<std::string>> texture2DMap;
    static std::unordered_map<std::vector<std::string>, GLuint, boost::hash<std::vector<std::string>>> textureCubeMapMap;
    static std::filesystem::path textureRoot;
};
