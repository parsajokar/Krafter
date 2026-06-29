#include "Krafter/Core/Event.h"
#include "Krafter/UILayer.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Renderer/Widgets.h"

namespace Krafter {

constexpr float k_UIOpacity = 0.6f;

UILayer::UILayer(Window& window, UIRenderer& renderer, Texture2D& uiTexture, Hotbar& hotbar)
    : Layer("UI")
    , m_Window(window)
    , m_Hotbar(hotbar)
    , m_Renderer(renderer)
    , m_UITexture(uiTexture)
    , m_BlockTexture("assets/textures/blocks.png")
{
}

void UILayer::OnRender()
{
    if (!m_Visible) {
        return;
    }

    m_Renderer.Begin();
    DrawHotbar();
    DrawCrosshair();
    m_Renderer.End();
}

void UILayer::OnEvent(Event& event)
{
    if (event.type != EventType::k_KeyPressed || event.isRepeat) {
        return;
    }

    // Keys 1-9 select slots 1-9; 0 selects the tenth slot.
    if (event.key >= Key::k_1 && event.key <= Key::k_9) {
        m_Hotbar.SetSelected(static_cast<int>(event.key) - static_cast<int>(Key::k_1));
        event.handled = true;
    } else if (event.key == Key::k_0) {
        m_Hotbar.SetSelected(Hotbar::k_SlotCount - 1);
        event.handled = true;
    }
}

void UILayer::DrawCrosshair()
{
    // The crosshair art is the bottom-left 15x15 of the UI texture.
    constexpr glm::vec2 k_SpriteSize = glm::vec2(15.0f, 15.0f);
    constexpr float k_Scale = 2.0f;

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 spritePos = glm::vec2(0.0f, texSize.y - k_SpriteSize.y);

    const glm::vec2 size = k_SpriteSize * k_Scale;
    // Snap to the pixel grid: an odd size centres on a half-pixel, which blurs
    // and visually shifts the sprite against the pixel-aligned hotbar.
    const glm::vec2 position = glm::floor(glm::vec2(m_Window.GetSize()) * 0.5f - size * 0.5f);

    // Inverts the colours behind it so it stays visible on any background.
    m_Renderer.DrawSpriteInverted(m_UITexture, spritePos, k_SpriteSize, position, size);
}

void UILayer::DrawHotbar()
{
    // The slot art is a 22x22 box at (15, 0) from the bottom-left of the texture.
    constexpr glm::vec2 k_SpriteSize = glm::vec2(22.0f, 22.0f);
    constexpr int k_SlotCount = Hotbar::k_SlotCount;
    constexpr float k_Spacing = 2.0f;
    constexpr float k_Scale = 2.0f;
    constexpr float k_Margin = 10.0f;

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 spritePos = glm::vec2(15.0f, texSize.y - k_SpriteSize.y);
    // Selection outline: a 22x22 box at (37, 0) from the bottom-left.
    const glm::vec2 outlineSpritePos = glm::vec2(37.0f, texSize.y - k_SpriteSize.y);

    const glm::vec2 slotSize = k_SpriteSize * k_Scale;
    const float stride = slotSize.x + k_Spacing * k_Scale;
    const float totalWidth = stride * k_SlotCount - k_Spacing * k_Scale;

    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());
    // Snap to the pixel grid so the bar lines up with the crosshair above it.
    const float startX = glm::floor((windowSize.x - totalWidth) * 0.5f);
    const float y = glm::floor(windowSize.y - slotSize.y - k_Margin);

    // Inset the icon so it sits inside the slot's border, then shrink it a touch
    // more and re-centre so the art doesn't crowd the slot edges.
    constexpr float k_IconInset = 4.0f * k_Scale;
    constexpr float k_IconScale = 0.87f;
    const glm::vec2 insetSize = slotSize - 2.0f * k_IconInset;
    const glm::vec2 iconSize = insetSize * k_IconScale;
    const glm::vec2 iconOffset = glm::vec2(k_IconInset) + (insetSize - iconSize) * 0.5f;

    for (int i = 0; i < k_SlotCount; ++i) {
        const glm::vec2 position = glm::vec2(startX + i * stride, y);
        m_Renderer.DrawSprite(m_UITexture, spritePos, k_SpriteSize, position, slotSize,
            glm::vec4(1.0f, 1.0f, 1.0f, k_UIOpacity));

        const Block block = m_Hotbar.GetBlock(i);
        if (block != Block::k_Air) {
            DrawBlockIcon(m_Renderer, m_BlockTexture, block, position + iconOffset, iconSize);
        }

        if (i == m_Hotbar.GetSelected()) {
            m_Renderer.DrawSprite(m_UITexture, outlineSpritePos, k_SpriteSize, position, slotSize);
        }
    }
}

} // namespace Krafter
