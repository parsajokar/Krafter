#pragma once

#include <string_view>

#include "glm/glm.hpp"

#include "Krafter/Item.h"
#include "Krafter/World/Block.h"

namespace Krafter {

class UIRenderer;
class Font;
class Texture2D;

bool RectContains(const glm::vec4& rect, const glm::vec2& point);

void DrawBlockIcon(
    UIRenderer& renderer, const Texture2D& blockTexture, Block block,
    const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

void DrawItemIcon(
    UIRenderer& renderer, const Texture2D& blockTexture, const Texture2D& itemTexture,
    const Item& item, const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

void DrawItemCount(
    UIRenderer& renderer, const Font& font, int count,
    const glm::vec2& position, const glm::vec2& size, float opacity = 1.0f);

void DrawSlot(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));
void DrawSlotOutline(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));
void DrawSlotSliced(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));
void DrawSlotOutlineSliced(
    UIRenderer& renderer, const Texture2D& uiTexture,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));

void DrawMenuButton(
    UIRenderer& renderer, const Font& font, const Texture2D& uiTexture,
    const glm::vec4& rect, std::string_view label, bool hovered);

}
