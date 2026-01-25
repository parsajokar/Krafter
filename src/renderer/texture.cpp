#include <iostream>

#include "glad/gl.h"

#include "stb_image.h"

#include "renderer/texture.h"

namespace Krafter {

Texture2D::Texture2D(std::string_view path)
{
    stbi_set_flip_vertically_on_load(true);

    int32_t channels_in_file;
    uint8_t* data = stbi_load(path.data(), &_size.x, &_size.y, &channels_in_file, 0);
    if (!data) {
        std::cerr << "[FILE] Could not read " << path << std::endl;
        assert(false);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &_id);

    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTextureStorage2D(_id, 1, GL_RGBA8, _size.x, _size.y);
    glTextureSubImage2D(_id, 0, 0, 0, _size.x, _size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &_id);
}

void Texture2D::Bind(uint32_t unit) const
{
    glBindTextureUnit(unit, _id);
}

} // namespace Krafter
