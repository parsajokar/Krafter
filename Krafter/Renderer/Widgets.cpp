#include "Krafter/Renderer/Widgets.h"

#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

namespace {

// The hotbar slot art reused as the button face, with the selection outline as
// the hover highlight: a 22x22 box at (15, 0) from the texture's bottom-left, the
// outline at (37, 0).
constexpr glm::vec2 k_SlotSprite = glm::vec2(22.0f, 22.0f);
constexpr float k_SlotSpriteX = 15.0f;
constexpr float k_OutlineSpriteX = 37.0f;

constexpr float k_ButtonTextScale = 1.0f;

} // namespace

bool RectContains(const glm::vec4& rect, const glm::vec2& point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z
        && point.y >= rect.y && point.y <= rect.y + rect.w;
}

void DrawMenuButton(
    UIRenderer& renderer, const Font& font, const Texture2D& uiTexture,
    const glm::vec4& rect, std::string_view label, bool hovered)
{
    const glm::vec2 texSize = glm::vec2(uiTexture.GetSize());
    const glm::vec2 slotPos = glm::vec2(k_SlotSpriteX, texSize.y - k_SlotSprite.y);
    const glm::vec2 outlinePos = glm::vec2(k_OutlineSpriteX, texSize.y - k_SlotSprite.y);

    const glm::vec2 position = glm::vec2(rect);
    const glm::vec2 size = glm::vec2(rect.z, rect.w);

    renderer.DrawSlicedSprite(uiTexture, slotPos, k_SlotSprite, position, size,
        glm::vec4(1.0f, 1.0f, 1.0f, hovered ? 0.85f : 0.6f));
    if (hovered) {
        renderer.DrawSlicedSprite(uiTexture, outlinePos, k_SlotSprite, position, size);
    }

    const glm::vec2 labelSize = glm::vec2(
        font.Measure(label, k_ButtonTextScale), font.LineHeight(k_ButtonTextScale));
    const glm::vec2 labelPos = glm::floor(position + (size - labelSize) * 0.5f);
    font.Draw(renderer, label, labelPos, k_ButtonTextScale, glm::vec4(1.0f));
}

} // namespace Krafter
