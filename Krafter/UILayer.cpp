#include "Krafter/UILayer.h"
#include "Krafter/Window.h"

namespace Krafter {

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
    DrawCrosshair();
    m_Renderer.End();
}

void UILayer::DrawCrosshair()
{
    constexpr glm::vec2 k_SpriteSize = glm::vec2(16.0f, 16.0f);
    constexpr float k_Scale = 2.0f;

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 k_SpritePos = glm::vec2(0.0f, texSize.y - k_SpriteSize.y);
    const glm::vec2 uvMin = k_SpritePos / texSize;
    const glm::vec2 uvMax = (k_SpritePos + k_SpriteSize) / texSize;
    // Textures are flipped vertically on load, so flip V to keep the sprite upright.
    const glm::vec4 uvRect = glm::vec4(
        uvMin.x, 1.0f - uvMin.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);

    const glm::vec2 size = k_SpriteSize * k_Scale;
    const glm::vec2 position = glm::vec2(m_Window.GetSize()) * 0.5f - size * 0.5f;

    m_Renderer.DrawQuad(position, size, m_UITexture, uvRect);
}

} // namespace Krafter
