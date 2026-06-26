#pragma once

#include <array>
#include <string_view>

#include "glm/glm.hpp"

#include "Krafter/Renderer/Texture.h"

namespace Krafter {

class UIRenderer;

// A bitmap font drawn from a 16x16 grid of glyph cells (CP437 layout: the glyph
// for byte `c` sits at column c % 16, row c / 16). Each glyph's drawn width is
// measured from the texture at load, so text is laid out proportionally rather
// than on a fixed monospace grid.
class Font {
public:
    Font(std::string_view path);

    // Width in pixels the text occupies when drawn at the given scale.
    float Measure(std::string_view text, float scale) const;

    // Height of a single line (one glyph cell) in pixels at the given scale.
    float LineHeight(float scale) const;

    // Draws text with its top-left corner at `position` (pixel space), tinted by
    // `color`. The glyphs are white, so the tint sets the text colour. When
    // `shadow` is set, a darkened copy is drawn one pixel down-right first, the
    // Minecraft-style drop shadow.
    void Draw(
        UIRenderer& renderer, std::string_view text,
        const glm::vec2& position, float scale, const glm::vec4& color, bool shadow = true) const;

private:
    // Draws the glyph run once at `position` with no shadow. Draw() layers this.
    void DrawGlyphs(
        UIRenderer& renderer, std::string_view text,
        const glm::vec2& position, float scale, const glm::vec4& color) const;

    static constexpr int k_Columns = 16;
    static constexpr float k_Spacing = 1.0f; // gap after each glyph, in cell pixels

    Texture2D m_Texture;
    int m_CellSize = 0; // side length of one glyph cell on the sheet, in pixels

    // Measured drawn width of each glyph, in cell pixels.
    std::array<float, 256> m_GlyphWidth {};
};

} // namespace Krafter
