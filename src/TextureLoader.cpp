#include "TextureLoader.hpp"

#include <IL/il.h>
#include <vector>
#include <algorithm>

std::filesystem::path TextureLoader::textureRoot = "assets/textures/";
std::unordered_map<std::string, GLuint> TextureLoader::textureMap;

GLuint TextureLoader::getTextureId(const std::filesystem::path &textureName){
    std::filesystem::path joinedPath = TextureLoader::textureRoot.string() + textureName.string();
    if (!std::filesystem::exists(joinedPath)){
        return 0;
    }
    std::string textureKey = std::filesystem::canonical(joinedPath);
    if (!TextureLoader::textureMap.count(textureKey)){
        TextureLoader::loadTexture(textureKey);
    }
    return TextureLoader::textureMap[textureKey];
}

void TextureLoader::freeTexture(const std::filesystem::path &textureName){
    std::string textureKey = std::filesystem::canonical(TextureLoader::textureRoot.string() + textureName.string());
    if (TextureLoader::textureMap.count(textureKey)){
        glDeleteTextures(1, &TextureLoader::textureMap[textureKey]);
        TextureLoader::textureMap.erase(textureKey);
    }
}

void TextureLoader::freeTextures(){
    std::vector<GLuint> textureIds(textureMap.size());
    std::transform(textureMap.begin(), textureMap.end(), textureIds.begin(), [] (std::pair<std::string, GLuint> p){return p.second;});
    glDeleteTextures(textureIds.size(), textureIds.data());
    TextureLoader::textureMap.clear();
}

void TextureLoader::setTextureRoot(const std::filesystem::path &newTextureRoot){
    if (!std::filesystem::exists(newTextureRoot)){
        return;
    }
    TextureLoader::textureRoot = newTextureRoot;
}

void TextureLoader::loadTexture(const std::string& textureKey){
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    ILuint loadedImage;
    ilGenImages(1, &loadedImage);
    ilBindImage(loadedImage);
    ilLoadImage(textureKey.c_str());
    int width = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    std::vector<std::byte> imageData(width * height * 4);
    ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA, IL_UNSIGNED_BYTE, imageData.data());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    ilDeleteImage(loadedImage);
    TextureLoader::textureMap[textureKey] = textureId;
}

