#include "load_texture.hpp"

#include <png.h>

#include <cstring>
#include <cmath>

std::vector<std::byte> getImageData(const std::filesystem::path& src, int& width, int& height) {
    if (!std::filesystem::exists(src)) {
        throw std::runtime_error("Failed to load image from " + src.string());
    }
    png_image loadedImage;
    std::memset(&loadedImage, 0, sizeof(loadedImage));
    loadedImage.version = PNG_IMAGE_VERSION;
    png_image_begin_read_from_file(&loadedImage, src.c_str());
    loadedImage.format = PNG_FORMAT_RGBA;
    std::vector<std::byte> imageData(PNG_IMAGE_SIZE(loadedImage));
    png_image_finish_read(&loadedImage, nullptr, imageData.data(), -PNG_IMAGE_ROW_STRIDE(loadedImage), nullptr);
    width = loadedImage.width;
    height = loadedImage.height;
    return imageData;
}

void loadTexture2D(GLuint texID, const std::filesystem::path& imagePath, GLenum texFormat, GLenum dataFormat, GLenum dataType) {
    int imageWidth;
    int imageHeight;
    auto imageData = getImageData(imagePath, imageWidth, imageHeight);
    int levels = std::floor(std::log2(std::max(imageWidth, imageHeight))) + 1;
    glTextureStorage2D(texID, levels, texFormat, imageWidth, imageHeight);
    glTextureSubImage2D(texID, 0, 0, 0, imageWidth, imageHeight, dataFormat, dataType, imageData.data());
    glGenerateTextureMipmap(texID);
}

void loadTextureCubeMap(GLuint texID, const std::vector<std::filesystem::path>& imagePaths, GLenum texFormat, GLenum dataFormat, GLenum dataType) {
    if (imagePaths.size() != 6){
        throw std::runtime_error("Cube maps have 6 faces, " + std::to_string(imagePaths.size()) + " provided");
    }
    int imageWidth;
    int imageHeight;
    std::vector<std::vector<std::byte>> imageData;
    for (std::size_t i = 0; i < 6; i++){
        imageData.push_back(getImageData(imagePaths[i], imageWidth, imageHeight));
    }
    int levels = std::floor(std::log2(std::max(imageWidth, imageHeight))) + 1;
    glTextureStorage2D(texID, levels, texFormat, imageWidth, imageHeight);
    for (std::size_t i = 0; i < 6; i++){
        glTextureSubImage3D(texID, 0, 0, 0, i, imageWidth, imageHeight, 1, dataFormat, dataType, imageData[i].data());
    }
    glGenerateTextureMipmap(texID);
}
