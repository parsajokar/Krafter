#include "Krafter/UILayer.h"
#include "Krafter/Window.h"

namespace Krafter {

constexpr float k_UIOpacity = 0.6f;

UILayer::UILayer(Window& window)
    : Layer("UI")
    , m_Window(window)
    , m_Renderer(window)
    , m_UITexture("assets/ui.png")
{
}

void UILayer::OnRender()
{
    m_Renderer.Begin();
    DrawHotbar();
    DrawCrosshair();
    m_Renderer.End();
}

void UILayer::DrawCrosshair()
{
    constexpr glm::vec2 k_SpriteSize = glm::vec2(16.0f, 16.0f);
    constexpr float k_Scale = 2.0f;

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 spritePos = glm::vec2(0.0f, texSize.y - k_SpriteSize.y);

    const glm::vec2 size = k_SpriteSize * k_Scale;
    const glm::vec2 position = glm::vec2(m_Window.GetSize()) * 0.5f - size * 0.5f;

    DrawSprite(spritePos, k_SpriteSize, position, size, k_UIOpacity);
}

void UILayer::DrawHotbar()
{
    constexpr glm::vec2 k_SpriteSize = glm::vec2(20.0f, 20.0f);
    constexpr int k_SlotCount = 10;
    constexpr float k_Spacing = 2.0f;
    constexpr float k_Scale = 2.0f;
    constexpr float k_Margin = 10.0f;

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 spritePos = glm::vec2(16.0f, texSize.y - k_SpriteSize.y);

    const glm::vec2 slotSize = k_SpriteSize * k_Scale;
    const float stride = slotSize.x + k_Spacing * k_Scale;
    const float totalWidth = stride * k_SlotCount - k_Spacing * k_Scale;

    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());
    const float startX = (windowSize.x - totalWidth) * 0.5f;
    const float y = windowSize.y - slotSize.y - k_Margin;

    for (int i = 0; i < k_SlotCount; ++i) {
        const glm::vec2 position = glm::vec2(startX + i * stride, y);
        DrawSprite(spritePos, k_SpriteSize, position, slotSize, k_UIOpacity);
    }
}

void UILayer::DrawSprite(
    const glm::vec2& spritePos, const glm::vec2& spriteSize,
    const glm::vec2& position, const glm::vec2& size, float opacity)
{
    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 uvMin = spritePos / texSize;
    const glm::vec2 uvMax = (spritePos + spriteSize) / texSize;
    // Textures are flipped vertically on load, so flip V to keep the sprite upright.
    const glm::vec4 uvRect = glm::vec4(
        uvMin.x, 1.0f - uvMin.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);

    m_Renderer.DrawQuad(position, size, m_UITexture, uvRect, glm::vec4(1.0f, 1.0f, 1.0f, opacity));
}

} // namespace Krafter
