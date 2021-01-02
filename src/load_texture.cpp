#include "load_texture.hpp"

#include <png.h>

#include <stdexcept>
#include <vector>

#include <cmath>

namespace {
struct ImageData {
    std::vector<std::byte> data;
    unsigned width;
    unsigned height;
    unsigned levels;
};

unsigned GetMipLevels(unsigned width, unsigned height) {
    unsigned m = std::max(width, height);
    unsigned c = 0;
    do {
        c++;
    } while (m >>= 1);
    return c;
}

ImageData GetImageData(std::string const& path) {
    png_image image = {.opaque = nullptr, .version = PNG_IMAGE_VERSION};
    if (!png_image_begin_read_from_file(&(image), path.c_str())) {
        return {};
    }
    image.format = PNG_FORMAT_RGBA;
    ImageData imageData = {.data = std::vector<std::byte>(PNG_IMAGE_SIZE(image))};
    if (!png_image_finish_read(&(image), nullptr, imageData.data.data(), -PNG_IMAGE_ROW_STRIDE(image), nullptr)) {
        png_image_free(&image);
        return {};
    }
    imageData.width = image.width;
    imageData.height = image.height;
    imageData.levels = GetMipLevels(imageData.width, imageData.height);
    return imageData;
}
};  // namespace

GLuint createTexture2D(std::string const& path, GLenum texFormat, GLenum dataFormat, GLenum dataType) {
    auto image = GetImageData(path);
    if (image.data.empty()) {
        throw std::runtime_error("Failed to load 2D texture from " + path);
    }
    GLuint texID = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texID);
    glTextureStorage2D(texID, image.levels, texFormat, image.width, image.height);
    glTextureSubImage2D(texID, 0, 0, 0, image.width, image.height, dataFormat, dataType, image.data.data());
    glGenerateTextureMipmap(texID);
    return texID;
}

GLuint createTextureCubeMap(const std::array<std::string, 6>& paths, GLenum texFormat, GLenum dataFormat, GLenum dataType) {
    std::array<ImageData, 6> images;
    for (std::size_t i = 0; i < 6; i++) {
        images[i] = GetImageData(paths[i]);
        if (images[i].data.empty()) {
            throw std::runtime_error("Failed to load cubemap face texture from " + paths[i]);
        }
    }

    GLuint texID = 0;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texID);
    glTextureStorage2D(texID, images[0].levels, texFormat, images[0].width, images[0].height);
    for (std::size_t i = 0; i < 6; i++) {
        glTextureSubImage3D(texID, 0, 0, 0, i, images[i].width, images[i].height, 1, dataFormat, dataType, images[i].data.data());
    }
    glGenerateTextureMipmap(texID);

    return texID;
}
