#pragma once

#include <array>
#include <string>
#include <string_view>

#include "glm/glm.hpp"

#include "Krafter/Engine/Graphics/Texture.h"

namespace Krafter {

class Renderer;
class UIRenderer;

class Font {
public:
    Font(Renderer& renderer, const std::string& path);

    float Measure(std::string_view text, float scale) const;

    float LineHeight(float scale) const;

    void Draw(
        UIRenderer& renderer, std::string_view text,
        const glm::vec2& position, float scale, const glm::vec4& color, bool shadow = true) const;

private:
    void DrawGlyphs(
        UIRenderer& renderer, std::string_view text,
        const glm::vec2& position, float scale, const glm::vec4& color) const;

    static constexpr int k_Columns = 16;
    static constexpr float k_Spacing = 1.0f;

    Texture2D m_Texture;
    int m_CellSize = 0;

    std::array<float, 256> m_GlyphWidth {};
};

}
