#include <array>

#include "Krafter/Event.h"
#include "Krafter/UILayer.h"
#include "Krafter/Window.h"

namespace Krafter {

constexpr float k_UIOpacity = 0.6f;

UILayer::UILayer(Window& window, Hotbar& hotbar)
    : Layer("UI")
    , m_Window(window)
    , m_Hotbar(hotbar)
    , m_Renderer(window)
    , m_UITexture("assets/ui.png")
    , m_BlockTexture("assets/texture.png")
{
}

void UILayer::OnRender()
{
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
    DrawSprite(spritePos, k_SpriteSize, position, size, 1.0f, true);
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

    // Inset the icon so it sits inside the slot's border.
    constexpr float k_IconInset = 4.0f * k_Scale;
    const glm::vec2 iconSize = slotSize - 2.0f * k_IconInset;

    for (int i = 0; i < k_SlotCount; ++i) {
        const glm::vec2 position = glm::vec2(startX + i * stride, y);
        DrawSprite(spritePos, k_SpriteSize, position, slotSize, k_UIOpacity);

        const Block block = m_Hotbar.GetBlock(i);
        if (block != Block::k_Air) {
            DrawBlockIcon(block, position + glm::vec2(k_IconInset), iconSize);
        }

        if (i == m_Hotbar.GetSelected()) {
            DrawSprite(outlineSpritePos, k_SpriteSize, position, slotSize);
        }
    }
}

void UILayer::DrawBlockIcon(Block block, const glm::vec2& position, const glm::vec2& size)
{
    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);

    // Atlas UVs are GPU coordinates (V from the bottom), so the tile's visual top
    // is at vMax. Corners are wound: top-left, top-right, bottom-right, bottom-left.
    auto tileUVs = [](const glm::vec2& tile) -> std::array<glm::vec2, 4> {
        const float uMin = tile.x;
        const float uMax = tile.x + BlockAtlas::k_Step;
        const float vMin = tile.y;
        const float vMax = tile.y + BlockAtlas::k_Step;
        return {
            glm::vec2(uMin, vMax), glm::vec2(uMax, vMax),
            glm::vec2(uMax, vMin), glm::vec2(uMin, vMin)
        };
    };

    // Isometric cube in a square box: top diamond is 2:1, side faces hang below.
    const float side = glm::min(size.x, size.y);
    const float cx = position.x + size.x * 0.5f;
    const float boxTop = position.y + (size.y - side) * 0.5f;

    const float w = side * 0.5f; // half width
    const float t = side * 0.25f; // top diamond half height
    const float h = side * 0.5f; // side face height
    const float midY = boxTop + t; // vertical centre of the top diamond

    const glm::vec2 top(cx, boxTop);
    const glm::vec2 right(cx + w, midY);
    const glm::vec2 front(cx, midY + t);
    const glm::vec2 left(cx - w, midY);
    const glm::vec2 frontBottom(cx, midY + t + h);
    const glm::vec2 leftBottom(cx - w, midY + h);
    const glm::vec2 rightBottom(cx + w, midY + h);

    // Shade faces like Minecraft: bright top, dimmer sides.
    constexpr glm::vec4 k_TopTint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    constexpr glm::vec4 k_LeftTint = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
    constexpr glm::vec4 k_RightTint = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

    m_Renderer.DrawQuad(
        { top, right, front, left }, tileUVs(atlas.top), m_BlockTexture, k_TopTint);
    m_Renderer.DrawQuad(
        { left, front, frontBottom, leftBottom }, tileUVs(atlas.side), m_BlockTexture, k_LeftTint);
    m_Renderer.DrawQuad(
        { front, right, rightBottom, frontBottom }, tileUVs(atlas.side), m_BlockTexture, k_RightTint);
}

void UILayer::DrawSprite(
    const glm::vec2& spritePos, const glm::vec2& spriteSize,
    const glm::vec2& position, const glm::vec2& size, float opacity, bool invert)
{
    const glm::vec2 texSize = glm::vec2(m_UITexture.GetSize());
    const glm::vec2 uvMin = spritePos / texSize;
    const glm::vec2 uvMax = (spritePos + spriteSize) / texSize;
    // Textures are flipped vertically on load, so flip V to keep the sprite upright.
    const glm::vec4 uvRect = glm::vec4(
        uvMin.x, 1.0f - uvMin.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);

    if (invert) {
        m_Renderer.DrawQuadInverted(position, size, m_UITexture, uvRect);
    } else {
        m_Renderer.DrawQuad(position, size, m_UITexture, uvRect, glm::vec4(1.0f, 1.0f, 1.0f, opacity));
    }
}

} // namespace Krafter
