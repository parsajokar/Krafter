#include "Krafter/Renderer/Widgets.h"

#include <array>
#include <string>

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

    // Items are laid out on a 16x16 grid in items.png, addressed by cell from the
    // sheet's bottom-left corner.
    constexpr float k_ItemTile = 16.0f;

    // Tool sprites carry transparent padding inside their tile, so drawn in the box
    // tuned for full-bleed block faces they look too small. Scale them up (centred on
    // the same spot) so the axe fills its slot like a block icon does.
    constexpr float k_ToolScale = 1.5f;

    glm::vec2 ItemCell(ItemKind kind)
    {
        switch (kind) {
        case ItemKind::k_WoodenAxe:
            return glm::vec2(0.0f, 0.0f);
        }
        return glm::vec2(0.0f, 0.0f);
    }

} // namespace

bool RectContains(const glm::vec4& rect, const glm::vec2& point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.z
        && point.y >= rect.y && point.y <= rect.y + rect.w;
}

void DrawItemCount(
    UIRenderer& renderer, const Font& font, int count,
    const glm::vec2& position, const glm::vec2& size, float opacity)
{
    // A single block or a tool needs no label; only real stacks are numbered.
    if (count <= 1) {
        return;
    }

    constexpr float k_CountScale = 1.0f;
    const std::string text = std::to_string(count);
    const glm::vec2 textSize = glm::vec2(
        font.Measure(text, k_CountScale), font.LineHeight(k_CountScale));

    // Tuck the number into the slot's bottom-right corner, a hair off the edge.
    constexpr float k_Pad = 1.0f;
    const glm::vec2 textPos = glm::floor(position + size - textSize - glm::vec2(k_Pad));
    font.Draw(renderer, text, textPos, k_CountScale, glm::vec4(1.0f, 1.0f, 1.0f, opacity));
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

void DrawBlockIcon(
    UIRenderer& renderer, const Texture2D& blockTexture, Block block,
    const glm::vec2& position, const glm::vec2& size, float opacity)
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

    // Every block draws as a flat sprite of its icon tile filling the box: the
    // side for most (grass keeps its fringe, foliage reads as in-world), but logs
    // show their end grain so they aren't mistaken for plain bark.
    const std::array<glm::vec2, 4> corners = {
        glm::vec2(position.x, position.y),
        glm::vec2(position.x + size.x, position.y),
        glm::vec2(position.x + size.x, position.y + size.y),
        glm::vec2(position.x, position.y + size.y)
    };

    // Representative oak-forest tints for the grayscale grass and leaf tiles.
    constexpr glm::vec4 k_GrassColor = glm::vec4(0.569f, 0.741f, 0.349f, 1.0f);
    constexpr glm::vec4 k_LeafColor = glm::vec4(0.471f, 0.671f, 0.302f, 1.0f);

    // Leaves, grass tufts, and ferns are grayscale and get tinted; the grass
    // block's side base is plain dirt (untinted) with its fringe added below.
    glm::vec4 tint = glm::vec4(1.0f);
    if (IsLeaves(block)) {
        tint = k_LeafColor;
    } else if (block == Block::k_ShortGrass || block == Block::k_Fern) {
        tint = k_GrassColor;
    }
    tint.a *= opacity;
    renderer.DrawQuad(corners, tileUVs(BlockIconTile(block)), blockTexture, tint);

    // The grass block's side is dirt with a biome-tinted fringe layered over it,
    // so draw that fringe on top to read green like the in-world block.
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

    // Tools are flat sprites in items.png, addressed from the sheet's bottom-left
    // like the rest of the HUD; the cell is measured from the bottom, so flip it.
    const glm::vec2 cell = ItemCell(item.kind);
    const glm::vec2 texSize = glm::vec2(itemTexture.GetSize());
    const glm::vec2 spritePos = glm::vec2(
        cell.x * k_ItemTile, texSize.y - (cell.y + 1.0f) * k_ItemTile);

    // Enlarge the sprite about the icon box's centre so the tool reads at a size
    // closer to a block icon despite its in-tile padding.
    const glm::vec2 toolSize = size * k_ToolScale;
    const glm::vec2 toolPos = position + (size - toolSize) * 0.5f;
    renderer.DrawSprite(itemTexture, spritePos, glm::vec2(k_ItemTile), toolPos, toolSize,
        glm::vec4(1.0f, 1.0f, 1.0f, opacity));
}

} // namespace Krafter
