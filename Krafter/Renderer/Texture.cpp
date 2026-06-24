#include <iostream>

#include "glad/gl.h"

#include "stb_image.h"

#include "Krafter/Renderer/Texture.h"

namespace Krafter {

Texture2D::Texture2D(std::string_view path)
{
    stbi_set_flip_vertically_on_load(true);

    int32_t channels_in_file;
    uint8_t* data = stbi_load(path.data(), &m_Size.x, &m_Size.y, &channels_in_file, 0);
    if (!data) {
        std::cerr << "[FILE] Could not read " << path << std::endl;
        assert(false);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_Id);

    glTextureParameteri(m_Id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_Id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_Id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_Id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTextureStorage2D(m_Id, 1, GL_RGBA8, m_Size.x, m_Size.y);
    glTextureSubImage2D(m_Id, 0, 0, 0, m_Size.x, m_Size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &m_Id);
}

void Texture2D::Bind(uint32_t unit) const
{
    glBindTextureUnit(unit, m_Id);
}

void Texture2D::UpdateRegion(int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels) const
{
    glTextureSubImage2D(m_Id, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

} // namespace Krafter
