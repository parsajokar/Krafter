#include <iostream>

#include "glad/gl.h"

#include "stb_image.h"

#include "Krafter/Renderer/Texture.h"

namespace Krafter {

Texture2D::Texture2D(std::string_view path)
{
    stbi_set_flip_vertically_on_load(true);

    int32_t channels_in_file;
    uint8_t* data = stbi_load(path.data(), &m_size.x, &m_size.y, &channels_in_file, 0);
    if (!data) {
        std::cerr << "[FILE] Could not read " << path << std::endl;
        assert(false);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_id);

    glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTextureStorage2D(m_id, 1, GL_RGBA8, m_size.x, m_size.y);
    glTextureSubImage2D(m_id, 0, 0, 0, m_size.x, m_size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
}

Texture2D::~Texture2D()
{
    glDeleteTextures(1, &m_id);
}

void Texture2D::Bind(uint32_t unit) const
{
    glBindTextureUnit(unit, m_id);
}

} // namespace Krafter
