#pragma once
#include "glad.h"

#include <boost/functional/hash.hpp>

#include <unordered_map>
#include <filesystem>

class TextureLoader {
public:
    TextureLoader() = delete;

    static GLuint getTextureId2D(const std::filesystem::path& textureName);
    template <typename S>
    static GLuint getTextureIdCubeMap(const S& textureNames);
    static void freeTexture2D(const std::filesystem::path& textureName);
    template <typename S>
    static void freeTextureCubeMap(const S& textureNames);
    static void freeTextures();
    static void setTextureRoot(const std::filesystem::path& newTextureRoot);

private:
    static void loadTexture2D(const std::string& textureKey);
    static void loadTextureCubeMap(const std::vector<std::string>& textureKey);

    static std::unordered_map<std::string, GLuint> texture2DMap;
    static std::unordered_map<std::vector<std::string>, GLuint, boost::hash<std::vector<std::string>>> textureCubeMapMap;
    static std::filesystem::path textureRoot;
};

template <typename S>
GLuint TextureLoader::getTextureIdCubeMap(const S& textureNames){
    static_assert(std::is_base_of_v<std::filesystem::path, typename S::value_type>, "textureNames must be a sequence of paths\n");
    if (textureNames.size() != 6){
        return 0;
    }
    std::vector<std::string> textureKey;
    for (const auto& textureName : textureNames){
        std::filesystem::path joinedPath = TextureLoader::textureRoot.string() + textureName.string();
        if (!std::filesystem::exists(joinedPath)){
            return 0;
        }
        textureKey.push_back(std::filesystem::canonical((joinedPath)));
    }
    if (!TextureLoader::textureCubeMapMap.count(textureKey)){
        TextureLoader::loadTextureCubeMap(textureKey);
    }
    return TextureLoader::textureCubeMapMap[textureKey];
}
