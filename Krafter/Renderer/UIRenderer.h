#pragma once

#include <array>
#include <cstdint>

#include "glm/glm.hpp"

#include "Krafter/Renderer/ShaderProgram.h"
#include "Krafter/Renderer/Texture.h"

namespace Krafter {

class Window;

// Draws quads in pixel space (origin top-left). Issue draws between Begin/End.
class UIRenderer {
public:
    UIRenderer(Window& window);
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    void Begin();
    void End();

    // Clips subsequent draws to a pixel rectangle (top-left origin, like the rest
    // of the UI). Used to keep overflowing text inside a field. Call ClearScissor
    // to resume drawing to the whole screen.
    void SetScissor(const glm::vec2& position, const glm::vec2& size);
    void ClearScissor();

    void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void DrawQuad(
        const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
        const glm::vec4& uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
        const glm::vec4& tint = glm::vec4(1.0f));

    // Draws a textured quad with four arbitrary corner positions (pixel space),
    // each paired with a UV. Corners are wound around the quad. Lets callers
    // build sheared shapes such as isometric block faces.
    void DrawQuad(
        const std::array<glm::vec2, 4>& corners, const std::array<glm::vec2, 4>& uvs,
        const Texture2D& texture, const glm::vec4& tint = glm::vec4(1.0f));

    // Draws a textured quad that inverts the colours behind it (Minecraft-style
    // crosshair). The texture must be white where opaque, transparent elsewhere.
    void DrawQuadInverted(
        const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
        const glm::vec4& uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

    // Draws a sub-rectangle of `texture` addressed from its bottom-left corner (in
    // pixels) and stretched to fill the destination rectangle. The HUD and menus
    // address their sprite sheet this way; the V axis is flipped so the sprite
    // stays upright despite textures loading vertically flipped.
    void DrawSprite(
        const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));

    // DrawSprite using the colour-inverting blend (for the crosshair).
    void DrawSpriteInverted(
        const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size);

    // Draws a sprite as a horizontal 3-slice so a small sprite makes a clean wide
    // button or field: the outer thirds (rounded caps) keep their aspect ratio
    // while only the middle third stretches.
    void DrawSlicedSprite(
        const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));

private:
    void DrawQuad(
        const glm::vec2& position, const glm::vec2& size,
        const glm::vec4& uvRect, const glm::vec4& tint, const Texture2D* texture);

    // The UV rectangle for a sprite addressed from the texture's bottom-left, with
    // the V axis flipped so it draws upright. Shared by the sprite helpers.
    static glm::vec4 SpriteUVRect(
        const glm::vec2& spritePos, const glm::vec2& spriteSize, const glm::vec2& texSize);

    Window& m_Window;

    ShaderProgram m_Program;

    uint32_t m_VertexArray;
    uint32_t m_VertexBuffer;
    uint32_t m_ElementBuffer;

    // Separate dynamic buffer for arbitrary-corner quads, re-uploaded per draw.
    uint32_t m_PolyVertexArray;
    uint32_t m_PolyVertexBuffer;
};

} // namespace Krafter
