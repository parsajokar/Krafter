#pragma once

#include <string_view>

#include "glm/glm.hpp"

#include "Krafter/Item.h"
#include "Krafter/World/Block.h"

namespace Krafter {

class UIRenderer;
class Font;
class Texture2D;

// True if `point` lies within the pixel rectangle (xy = top-left, zw = size).
bool RectContains(const glm::vec4& rect, const glm::vec2& point);

// Draws `block`'s side tile as a flat icon filling the pixel rectangle at
// `position` (top-left) with `size`, biome-tinting the grayscale foliage tiles
// so they read in colour. `blockTexture` is the world block atlas (blocks.png).
// Shared by the hotbar HUD and the inventory screen for their slot icons.
void DrawBlockIcon(
    UIRenderer& renderer, const Texture2D& blockTexture, Block block,
    const glm::vec2& position, const glm::vec2& size);

// Draws a slot's contents as an icon filling the pixel rectangle at `position`
// (top-left) with `size`: a block draws its tile from `blockTexture` (as
// DrawBlockIcon), while a tool draws its flat sprite from the item sheet
// (items.png). Shared by the hotbar HUD and the inventory screen.
void DrawItemIcon(
    UIRenderer& renderer, const Texture2D& blockTexture, const Texture2D& itemTexture,
    const Item& item, const glm::vec2& position, const glm::vec2& size);

// Draws a stack count in the bottom-right of the slot box at `position` with
// `size`, the way Minecraft labels a stack. Drawn only when `count` is above one,
// so a lone block or a tool shows no number. `font` is the shared HUD font.
void DrawItemCount(
    UIRenderer& renderer, const Font& font, int count,
    const glm::vec2& position, const glm::vec2& size);

// Draws a menu button: the UI slot sprite 9-sliced to `rect`, with a brighter
// face and a selection outline when `hovered`, and `label` centred inside.
// `uiTexture` is the shared UI sprite sheet (ui.png).
void DrawMenuButton(
    UIRenderer& renderer, const Font& font, const Texture2D& uiTexture,
    const glm::vec4& rect, std::string_view label, bool hovered);

} // namespace Krafter
