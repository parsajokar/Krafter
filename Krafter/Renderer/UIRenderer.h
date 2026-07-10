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

class UIRenderer {
public:
    UIRenderer(Window& window);
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    void Begin();
    void End();

    void SetScissor(const glm::vec2& position, const glm::vec2& size);
    void ClearScissor();

    void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void DrawQuad(
        const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
        const glm::vec4& uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
        const glm::vec4& tint = glm::vec4(1.0f));

    void DrawQuad(
        const std::array<glm::vec2, 4>& corners, const std::array<glm::vec2, 4>& uvs,
        const Texture2D& texture, const glm::vec4& tint = glm::vec4(1.0f));

    void DrawQuadInverted(
        const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
        const glm::vec4& uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

    void DrawSprite(
        const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));

    void DrawSpriteInverted(
        const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size);

    void DrawSlicedSprite(
        const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
        const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint = glm::vec4(1.0f));

private:
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

    static glm::vec4 SpriteUVRect(
        const glm::vec2& spritePos, const glm::vec2& spriteSize, const glm::vec2& texSize);

    static constexpr uint32_t k_MaxDynamicVertices = 4096;

    Window& m_Window;

    std::unique_ptr<Pipeline> m_Pipeline;
    std::unique_ptr<Pipeline> m_InvertPipeline;

    GpuBuffer m_QuadVertexBuffer;
    GpuBuffer m_QuadIndexBuffer;

    std::unique_ptr<Texture2D> m_WhiteTexture;

    struct DynamicBuffer {
        GpuBuffer buffer;
        void* mapped = nullptr;
    };
    std::array<DynamicBuffer, Renderer::k_MaxFramesInFlight> m_Dynamic;
    uint32_t m_DynamicVertexOffset = 0;
    uint32_t m_DynamicFrame = UINT32_MAX;

    glm::mat4 m_Projection = glm::mat4(1.0f);
};

}
