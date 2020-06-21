#pragma once
#include "glad.h"

#include <unordered_map>
#include <filesystem>

class TextureLoader {
public:
    TextureLoader() = delete;

    static GLuint getTextureId(const std::filesystem::path& textureName);
    static void freeTexture(const std::filesystem::path& textureName);
    static void freeTextures();
    static void setTextureRoot(const std::filesystem::path& newTextureRoot);

private:
    static void loadTexture(const std::string& textureName);

    static std::unordered_map<std::string, GLuint> textureMap;
    static std::filesystem::path textureRoot;
};
