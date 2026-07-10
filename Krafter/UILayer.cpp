#include "Krafter/UILayer.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Renderer/UIRenderer.h"
#include "Krafter/Renderer/Widgets.h"

namespace Krafter {

constexpr float k_UIOpacity = 0.6f;

UILayer::UILayer(Window& window, UIRenderer& renderer, Texture2D& uiTexture, Font& font, Hotbar& hotbar)
    : UIScreen("UI", window, renderer, uiTexture, font)
    , m_Hotbar(hotbar)
    , m_BlockTexture("assets/textures/blocks.png")
    , m_ItemTexture("assets/textures/items.png")
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
    constexpr glm::vec2 k_SpriteSize = glm::vec2(15.0f, 15.0f);
    constexpr float k_Scale = 2.0f;

    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 spritePos = glm::vec2(0.0f, texSize.y - k_SpriteSize.y);

    const glm::vec2 size = k_SpriteSize * k_Scale;
    const glm::vec2 position = glm::floor(glm::vec2(m_Window.GetSize()) * 0.5f - size * 0.5f);

    m_Renderer.DrawSpriteInverted(m_UITexture, spritePos, k_SpriteSize, position, size);
}

void UILayer::DrawHotbar()
{
    constexpr glm::vec2 k_SpriteSize = glm::vec2(22.0f, 22.0f);
    constexpr int k_SlotCount = Hotbar::k_SlotCount;
    constexpr float k_Spacing = 2.0f;
    constexpr float k_Scale = 2.0f;
    constexpr float k_Margin = 10.0f;

    const glm::vec2 slotSize = k_SpriteSize * k_Scale;
    const float stride = slotSize.x + k_Spacing * k_Scale;
    const float totalWidth = stride * k_SlotCount - k_Spacing * k_Scale;

    const glm::vec2 windowSize = glm::vec2(m_Window.GetSize());
    const float startX = glm::floor((windowSize.x - totalWidth) * 0.5f);
    const float y = glm::floor(windowSize.y - slotSize.y - k_Margin);

    constexpr float k_IconInset = 4.0f * k_Scale;
    constexpr float k_IconScale = 0.87f;
    const glm::vec2 insetSize = slotSize - 2.0f * k_IconInset;
    const glm::vec2 iconSize = insetSize * k_IconScale;
    const glm::vec2 iconOffset = glm::vec2(k_IconInset) + (insetSize - iconSize) * 0.5f;

    for (int i = 0; i < k_SlotCount; ++i) {
        const glm::vec2 position = glm::vec2(startX + i * stride, y);
        DrawSlot(m_Renderer, m_UITexture, position, slotSize,
            glm::vec4(1.0f, 1.0f, 1.0f, k_UIOpacity));

        if (i == m_Hotbar.GetSelected()) {
            DrawSlotOutline(m_Renderer, m_UITexture, position, slotSize);
        }

        const Item item = m_Hotbar.GetItem(i);
        if (!item.IsEmpty()) {
            DrawItemIcon(m_Renderer, m_BlockTexture, m_ItemTexture, item,
                position + iconOffset, iconSize);
            DrawItemCount(m_Renderer, m_Font, item.count, position, slotSize);
        }
    }
}

}
