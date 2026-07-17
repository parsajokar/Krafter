#include "Krafter/Renderer/Widgets.h"

#include <array>
#include <string>

#include "Krafter/Renderer/Font.h"
#include "Krafter/Renderer/Texture.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

namespace {

constexpr glm::vec2 k_SlotSprite = glm::vec2(22.0f, 22.0f);
constexpr float k_SlotSpriteX = 15.0f;
constexpr float k_OutlineSpriteX = 37.0f;

glm::vec2 SlotSpritePos(const Texture2D& uiTexture)
{
    return glm::vec2(k_SlotSpriteX, uiTexture.GetSize().y - k_SlotSprite.y);
}
glm::vec2 OutlineSpritePos(const Texture2D& uiTexture)
{
    return glm::vec2(k_OutlineSpriteX, uiTexture.GetSize().y - k_SlotSprite.y);
}

constexpr float k_ButtonTextScale = 1.0f;

constexpr float k_ItemTile = 16.0f;

constexpr float k_ToolScale = 1.5f;

glm::vec2 ItemCell(ItemKind kind)
{
    switch (kind) {
    case ItemKind::k_WoodenAxe:
        return glm::vec2(0.0f, 0.0f);
    case ItemKind::k_WoodenPickaxe:
        return glm::vec2(1.0f, 0.0f);
    case ItemKind::k_WoodenShovel:
        return glm::vec2(2.0f, 0.0f);
    case ItemKind::k_WoodenSword:
        return glm::vec2(3.0f, 0.0f);
    }
    return glm::vec2(0.0f, 0.0f);
}

}

bool RectContains(const glm::vec4& rect, const glm::vec2& point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z
        && point.y >= rect.y && point.y <= rect.y + rect.w;
}

void DrawItemCount(
    UIRenderer& renderer, const Font& font, int count,
    const glm::vec2& position, const glm::vec2& size, float opacity)
{
    if (count <= 1) {
        return;
    }

    constexpr float k_CountScale = 1.0f;
    const std::string text = std::to_string(count);
    const glm::vec2 textSize = glm::vec2(
        font.Measure(text, k_CountScale), font.LineHeight(k_CountScale));

    constexpr float k_Pad = 1.0f;
    const glm::vec2 textPos = glm::floor(position + size - textSize - glm::vec2(k_Pad));
    font.Draw(renderer, text, textPos, k_CountScale, glm::vec4(1.0f, 1.0f, 1.0f, opacity));
}

void DrawSlot(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    renderer.DrawSprite(uiTexture, SlotSpritePos(uiTexture), k_SlotSprite, position, size, tint);
}

void DrawSlotOutline(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    renderer.DrawSprite(uiTexture, OutlineSpritePos(uiTexture), k_SlotSprite, position, size, tint);
}

void DrawSlotSliced(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    renderer.DrawSlicedSprite(uiTexture, SlotSpritePos(uiTexture), k_SlotSprite, position, size, tint);
}

void DrawSlotOutlineSliced(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    renderer.DrawSlicedSprite(uiTexture, OutlineSpritePos(uiTexture), k_SlotSprite, position, size, tint);
}

void DrawMenuButton(
    UIRenderer& renderer, const Font& font, const Texture2D& uiTexture,
    const glm::vec4& rect, std::string_view label, bool hovered)
{
    const glm::vec2 position = glm::vec2(rect);
    const glm::vec2 size = glm::vec2(rect.z, rect.w);

    DrawSlotSliced(renderer, uiTexture, position, size,
        glm::vec4(1.0f, 1.0f, 1.0f, hovered ? 0.85f : 0.6f));
    if (hovered) {
        DrawSlotOutlineSliced(renderer, uiTexture, position, size);
    }

    const glm::vec2 labelSize = glm::vec2(
        font.Measure(label, k_ButtonTextScale), font.LineHeight(k_ButtonTextScale));
    const glm::vec2 labelPos = glm::floor(position + (size - labelSize) * 0.5f);
    font.Draw(renderer, label, labelPos, k_ButtonTextScale, glm::vec4(1.0f));
}

void DrawBlockIcon(
    UIRenderer& renderer, const Texture2D& blockTexture, Block block,
    const glm::vec2& position, const glm::vec2& size, float opacity)
{
    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);

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

    const std::array<glm::vec2, 4> corners = {
        glm::vec2(position.x, position.y),
        glm::vec2(position.x + size.x, position.y),
        glm::vec2(position.x + size.x, position.y + size.y),
        glm::vec2(position.x, position.y + size.y)
    };

    constexpr glm::vec4 k_GrassColor = glm::vec4(0.569f, 0.741f, 0.349f, 1.0f);
    constexpr glm::vec4 k_LeafColor = glm::vec4(0.471f, 0.671f, 0.302f, 1.0f);

    glm::vec4 tint = glm::vec4(1.0f);
    if (IsLeaves(block)) {
        tint = k_LeafColor;
    } else if (block == Block::k_ShortGrass || block == Block::k_Fern) {
        tint = k_GrassColor;
    }
    tint.a *= opacity;
    renderer.DrawQuad(corners, tileUVs(BlockIconTile(block)), blockTexture, tint);

    if (block == Block::k_Grass) {
        const glm::vec4 fringe = glm::vec4(glm::vec3(k_GrassColor), k_GrassColor.a * opacity);
        renderer.DrawQuad(corners, tileUVs(atlas.sideOverlay), blockTexture, fringe);
    }
}

void DrawItemIcon(
    UIRenderer& renderer, const Texture2D& blockTexture, const Texture2D& itemTexture,
    const Item& item, const glm::vec2& position, const glm::vec2& size, float opacity)
{
    if (!item.isItem) {
        DrawBlockIcon(renderer, blockTexture, item.block, position, size, opacity);
        return;
    }

    const glm::vec2 cell = ItemCell(item.kind);
    const glm::vec2 texSize = glm::vec2(itemTexture.GetSize());
    const glm::vec2 spritePos = glm::vec2(
        cell.x * k_ItemTile, texSize.y - (cell.y + 1.0f) * k_ItemTile);

    const glm::vec2 toolSize = size * k_ToolScale;
    const glm::vec2 toolPos = position + (size - toolSize) * 0.5f;
    renderer.DrawSprite(itemTexture, spritePos, glm::vec2(k_ItemTile), toolPos, toolSize,
        glm::vec4(1.0f, 1.0f, 1.0f, opacity));
}

}
