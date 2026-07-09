#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "glm/glm.hpp"

#include "Krafter/Core/Renderer.h"
#include "Krafter/Renderer/Pipeline.h"
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
    // Per-draw constants matching the ui shaders' push_constant block.
    struct UIPush {
        glm::mat4 projection;
        glm::vec4 transform;
        glm::vec4 uvRect;
        glm::vec4 tint;
        int32_t useTexture;
        int32_t invert;
    };
    static_assert(sizeof(UIPush) == 120, "UIPush must match the ui shaders");

    void DrawQuad(
        const glm::vec2& position, const glm::vec2& size,
        const glm::vec4& uvRect, const glm::vec4& tint, const Texture2D* texture);

    // The UV rectangle for a sprite addressed from the texture's bottom-left, with
    // the V axis flipped so it draws upright. Shared by the sprite helpers.
    static glm::vec4 SpriteUVRect(
        const glm::vec2& spritePos, const glm::vec2& spriteSize, const glm::vec2& texSize);

    // Vertices the current frame's dynamic ring can hold (four per arbitrary quad).
    static constexpr uint32_t k_MaxDynamicVertices = 4096;

    Window& m_Window;

    // Two pipelines: standard alpha blending and the colour-inverting crosshair blend.
    std::unique_ptr<Pipeline> m_Pipeline;
    std::unique_ptr<Pipeline> m_InvertPipeline;

    // Static unit quad (0..1 position and uv) transformed per draw by push constants.
    GpuBuffer m_QuadVertexBuffer;
    GpuBuffer m_QuadIndexBuffer;

    // 1x1 white texture bound for untextured draws so the shared descriptor set is
    // always valid (the shader ignores it when useTexture is 0).
    std::unique_ptr<Texture2D> m_WhiteTexture;

    // Per-frame ring of host-visible vertex buffers for arbitrary-corner quads,
    // written directly each draw. One per frame-in-flight so a buffer is never
    // overwritten while the GPU still reads it.
    struct DynamicBuffer {
        GpuBuffer buffer;
        void* mapped = nullptr;
    };
    std::array<DynamicBuffer, Renderer::k_MaxFramesInFlight> m_Dynamic;
    uint32_t m_DynamicVertexOffset = 0;
    uint32_t m_DynamicFrame = UINT32_MAX;

    glm::mat4 m_Projection = glm::mat4(1.0f);
};

} // namespace Krafter
