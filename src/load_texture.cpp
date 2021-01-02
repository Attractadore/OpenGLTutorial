#include "load_texture.hpp"

#include <png.h>

#include <stdexcept>
#include <vector>

#include <cmath>

struct ImageDimensions {
    unsigned width, height;
};

class PNGImage {
public:
    PNGImage(const std::filesystem::path& path);
    PNGImage(const PNGImage& other) = delete;
    PNGImage(PNGImage&& other) noexcept;

    ~PNGImage();

    PNGImage& operator=(const PNGImage& other) = delete;
    PNGImage& operator=(PNGImage&& other) noexcept;

    bool IsOpened() const noexcept;

    char const* GetMessageCStr() const noexcept;

    ImageDimensions GetDimensions() const noexcept;

    std::vector<std::byte> const& GetData() const noexcept;

private:
    png_image image;
    std::vector<std::byte> data;
};

PNGImage::PNGImage(std::filesystem::path const& path) {
    this->image = {.opaque = nullptr, .version = PNG_IMAGE_VERSION};
    if (png_image_begin_read_from_file(&(this->image), path.string().c_str()) != 0) {
        this->image.format = PNG_FORMAT_RGBA;
        this->data.resize(PNG_IMAGE_SIZE(this->image));
        png_image_finish_read(&(this->image), nullptr, this->data.data(), -PNG_IMAGE_ROW_STRIDE(this->image), nullptr);
    }
}

PNGImage::PNGImage(PNGImage&& other) noexcept:
    data{std::move(other.data)} {
    this->image = other.image;
    other.image = {.opaque = nullptr, .version = PNG_IMAGE_VERSION};
}

PNGImage::~PNGImage() {
    png_image_free(&this->image);
}

PNGImage& PNGImage::operator=(PNGImage&& other) noexcept {
    png_image_free(&this->image);
    this->image = other.image;
    other.image = {.opaque = nullptr, .version = PNG_IMAGE_VERSION};
    this->data = std::move(other.data);
    return *this;
}

bool PNGImage::IsOpened() const noexcept {
    return this->image.warning_or_error | PNG_IMAGE_ERROR;
}

ImageDimensions PNGImage::GetDimensions() const noexcept {
    return {.width = this->image.width, .height = this->image.height};
}

char const* PNGImage::GetMessageCStr() const noexcept {
    return this->image.message;
}

std::vector<std::byte> const& PNGImage::GetData() const noexcept {
    return this->data;
}

static unsigned GetMipLevels(ImageDimensions const& dimensions) {
    return std::floor(std::log2(std::max(dimensions.width, dimensions.height))) + 1;
}

GLuint createTexture2D(const std::filesystem::path& path, GLenum texFormat, GLenum dataFormat, GLenum dataType) {
    PNGImage image(path);
    if (!image.IsOpened()) {
        throw std::runtime_error("Failed to create 2D texture from" + path.string() + ":\n" + image.GetMessageCStr());
    }
    auto dimensions = image.GetDimensions();
    auto levels = GetMipLevels(dimensions);
    auto data = image.GetData();
    GLuint texID = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texID);
    glTextureStorage2D(texID, levels, texFormat, dimensions.width, dimensions.height);
    glTextureSubImage2D(texID, 0, 0, 0, dimensions.width, dimensions.height, dataFormat, dataType, data.data());
    glGenerateTextureMipmap(texID);
    return texID;
}

GLuint createTextureCubeMap(const std::array<std::filesystem::path, 6>& paths, GLenum texFormat, GLenum dataFormat, GLenum dataType) {
    std::vector<PNGImage> images;
    for (const auto& path: paths) {
        images.emplace_back(path);
        PNGImage const& image = images.back();
        if (!image.IsOpened()) {
            throw std::runtime_error("Failed to create cubemap texture face from" + path.string() + ":\n" + image.GetMessageCStr());
        }
    }

    auto dimensions = images.front().GetDimensions();
    auto levels = GetMipLevels(dimensions);
    GLuint texID = 0;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texID);
    glTextureStorage2D(texID, levels, texFormat, dimensions.width, dimensions.height);
    for (std::size_t i = 0; i < 6; i++) {
        glTextureSubImage3D(texID, 0, 0, 0, i, dimensions.width, dimensions.height, 1, dataFormat, dataType, images[i].GetData().data());
    }

    glGenerateTextureMipmap(texID);
    return texID;
}
