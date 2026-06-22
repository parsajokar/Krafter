#include <array>

#include "Krafter/UILayer.h"
#include "Krafter/Window.h"

namespace Krafter {

constexpr float k_UIOpacity = 0.6f;

UILayer::UILayer(Window& window)
    : Layer("UI")
    , m_Window(window)
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

    // Blocks shown in each slot; k_Air leaves the slot empty.
    constexpr Block k_SlotBlocks[k_SlotCount] = {
        Block::k_Grass, Block::k_Sand
    };

    // Inset the icon so it sits inside the slot's border.
    constexpr float k_IconInset = 4.0f * k_Scale;
    const glm::vec2 iconSize = slotSize - 2.0f * k_IconInset;

    for (int i = 0; i < k_SlotCount; ++i) {
        const glm::vec2 position = glm::vec2(startX + i * stride, y);
        DrawSprite(spritePos, k_SpriteSize, position, slotSize, k_UIOpacity);

        if (k_SlotBlocks[i] != Block::k_Air) {
            DrawBlockIcon(k_SlotBlocks[i], position + glm::vec2(k_IconInset), iconSize);
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
