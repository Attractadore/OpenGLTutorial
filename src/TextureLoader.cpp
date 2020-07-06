#include "TextureLoader.hpp"

#include <IL/il.h>
#include <vector>
#include <algorithm>

std::filesystem::path TextureLoader::textureRoot = "assets/textures/";
std::unordered_map<std::string, GLuint> TextureLoader::texture2DMap;
std::unordered_map<std::vector<std::string>, GLuint, boost::hash<std::vector<std::string>>> TextureLoader::textureCubeMapMap;

GLuint TextureLoader::getTextureId2D(const std::filesystem::path &textureName){
    std::filesystem::path joinedPath = TextureLoader::textureRoot.string() + textureName.string();
    if (!std::filesystem::exists(joinedPath)){
        return 0;
    }
    std::string textureKey = std::filesystem::canonical(joinedPath);
    if (!TextureLoader::texture2DMap.count(textureKey)){
        TextureLoader::loadTexture2D(textureKey);
    }
    return TextureLoader::texture2DMap[textureKey];
}

void TextureLoader::freeTexture2D(const std::filesystem::path &textureName){
    std::string textureKey = std::filesystem::canonical(TextureLoader::textureRoot.string() + textureName.string());
    if (TextureLoader::texture2DMap.count(textureKey)){
        glDeleteTextures(1, &TextureLoader::texture2DMap[textureKey]);
        TextureLoader::texture2DMap.erase(textureKey);
    }
}

void TextureLoader::setTextureRoot(const std::filesystem::path &newTextureRoot){
    if (!std::filesystem::exists(newTextureRoot)){
        return;
    }
    TextureLoader::textureRoot = newTextureRoot;
}

std::vector<std::byte> getImageData(const std::string& src, int& width, int& height){
    ILuint loadedImage;
    ilGenImages(1, &loadedImage);
    ilBindImage(loadedImage);
    ilLoadImage(src.c_str());
    width = ilGetInteger(IL_IMAGE_WIDTH);
    height = ilGetInteger(IL_IMAGE_HEIGHT);
    std::vector<std::byte> imageData(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, imageData.data());
    ilDeleteImage(loadedImage);
    return imageData;
}

void TextureLoader::loadTexture2D(const std::string& textureKey){
    GLint currentBoundTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentBoundTexture);
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    int width, height;
    auto imageData = getImageData(textureKey, width, height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    TextureLoader::texture2DMap[textureKey] = textureId;
    glBindTexture(GL_TEXTURE_2D, currentBoundTexture);
}

void TextureLoader::loadTextureCubeMap(const std::vector<std::string>& textureKey){
    GLint currentBoundTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &currentBoundTexture);
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    for (int i = 0; i < 6; i++){
        int width, height;
        auto imageData = getImageData(textureKey[i], width, height);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    TextureLoader::textureCubeMapMap[textureKey] = textureId;
    glBindTexture(GL_TEXTURE_CUBE_MAP, currentBoundTexture);

}

