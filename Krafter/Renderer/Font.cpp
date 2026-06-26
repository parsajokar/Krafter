#include <cstdint>

#include "stb_image.h"

#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

Font::Font(std::string_view path)
    : m_Texture(path)
{
    m_CellSize = m_Texture.GetSize().x / k_Columns;

    // Measure each glyph by finding its rightmost opaque column. The image is
    // loaded top-down (unflipped) here so its rows line up with the CP437 grid;
    // Texture2D always restores the flipped-load default the GPU textures use.
    stbi_set_flip_vertically_on_load(false);

    int width = 0, height = 0, channels = 0;
    uint8_t* pixels = stbi_load(path.data(), &width, &height, &channels, 4);

    for (int c = 0; c < 256; ++c) {
        const int cellX = (c % k_Columns) * m_CellSize;
        const int cellY = (c / k_Columns) * m_CellSize;

        int rightmost = -1;
        if (pixels) {
            for (int y = 0; y < m_CellSize; ++y) {
                for (int x = 0; x < m_CellSize; ++x) {
                    const uint8_t alpha = pixels[((cellY + y) * width + (cellX + x)) * 4 + 3];
                    if (alpha > 16 && x > rightmost) {
                        rightmost = x;
                    }
                }
            }
        }

        // Blank cells (the space glyph, control codes) get a half-cell advance so
        // words still have a gap between them.
        m_GlyphWidth[c] = rightmost >= 0 ? static_cast<float>(rightmost + 1) : m_CellSize * 0.5f;
    }

    if (pixels) {
        stbi_image_free(pixels);
    }

    stbi_set_flip_vertically_on_load(true);
}

float Font::Measure(std::string_view text, float scale) const
{
    float width = 0.0f;
    for (char ch : text) {
        width += (m_GlyphWidth[static_cast<uint8_t>(ch)] + k_Spacing) * scale;
    }
    // Drop the trailing gap so the measure matches the visible glyphs exactly.
    if (!text.empty()) {
        width -= k_Spacing * scale;
    }
    return width;
}

float Font::LineHeight(float scale) const
{
    return m_CellSize * scale;
}

void Font::Draw(
    UIRenderer& renderer, std::string_view text,
    const glm::vec2& position, float scale, const glm::vec4& color, bool shadow) const
{
    if (shadow) {
        // A darkened copy one source-pixel down-right, like Minecraft's font.
        const glm::vec4 shadowColor = glm::vec4(glm::vec3(color) * 0.25f, color.a);
        DrawGlyphs(renderer, text, position + glm::vec2(scale), scale, shadowColor);
    }
    DrawGlyphs(renderer, text, position, scale, color);
}

void Font::DrawGlyphs(
    UIRenderer& renderer, std::string_view text,
    const glm::vec2& position, float scale, const glm::vec4& color) const
{
    const glm::vec2 texSize = glm::vec2(m_Texture.GetSize());
    const float cell = static_cast<float>(m_CellSize);
    const glm::vec2 quadSize = glm::vec2(cell * scale);

    float penX = position.x;
    for (char ch : text) {
        const uint8_t c = static_cast<uint8_t>(ch);
        const glm::vec2 cellPos = glm::vec2(c % k_Columns, c / k_Columns) * cell;

        // Textures load vertically flipped, so flip V to keep the glyph upright,
        // matching how UILayer samples its sprites.
        const glm::vec2 uvMin = cellPos / texSize;
        const glm::vec2 uvMax = (cellPos + cell) / texSize;
        const glm::vec4 uvRect = glm::vec4(
            uvMin.x, 1.0f - uvMin.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);

        renderer.DrawQuad(glm::vec2(penX, position.y), quadSize, m_Texture, uvRect, color);

        penX += (m_GlyphWidth[c] + k_Spacing) * scale;
    }
}

} // namespace Krafter
