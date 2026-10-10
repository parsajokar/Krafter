#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Engine/Graphics/Pipeline.h"
#include "Krafter/Engine/Graphics/Texture.h"
#include "Krafter/Rendering/ChunkMesh.h"
#include "Krafter/Rendering/RenderView.h"

namespace Krafter {

class Renderer;
class Sky;
class DropSystem;

class WorldRenderer {
public:
    explicit WorldRenderer(Renderer& renderer);

    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;

    void BeginChunkPass(ChunkPass pass, const RenderView& view, const Sky& sky);

    struct ChunkInstance {
        glm::vec3 origin;
        float pad;
    };

    void AnimateWater();

    void AnimateLava();

    void RenderBlockOutline(const glm::ivec3& blockPosition, const RenderView& view);

    void RenderBlockBreak(const glm::ivec3& blockPosition, float progress, const RenderView& view, bool cross = false);

    void RenderDrops(const DropSystem& drops, const RenderView& view, float alpha);

    void RenderImGui();

private:
    struct ChunkPush {
        glm::mat4 viewProjection;
        glm::vec3 sunColor;
        float alphaScale;
        glm::vec3 sunDirection;
        float isWater;
        glm::vec3 ambientColor;
        float pad;
    };
    static_assert(sizeof(ChunkPush) == 112,
        "ChunkPush must match the shader's push_constant block layout");

    struct OutlinePush {
        glm::mat4 viewProjection;
        glm::vec3 blockPosition;
        float pad;
    };
    static_assert(sizeof(OutlinePush) == 80, "OutlinePush must match outline.vert");

    struct BreakPush {
        glm::mat4 viewProjection;
        glm::vec3 blockPosition;
        float frameBase;
        float frameSpan;
    };
    static_assert(sizeof(BreakPush) == 84, "BreakPush must match break.vert");

    struct ItemPush {
        glm::mat4 viewProjection;
        glm::vec3 center;
        float pad0;
        glm::vec3 right;
        float pad1;
        glm::vec3 up;
        float pad2;
        glm::vec3 tile;
        float pad3;
    };
    static_assert(sizeof(ItemPush) == 128, "ItemPush must match item.vert");

    ChunkPush MakeChunkPush(const glm::mat4& viewProjection, const Sky& sky, float alphaScale, float isWater) const;

    void RenderItemDrop(
        const glm::vec3& center, const glm::vec3& right, const glm::vec3& up,
        const glm::vec2& tileOrigin, bool fromItemAtlas, const glm::mat4& viewProjection);

    void AnimateTile(const std::vector<uint8_t>& frames, int32_t frameCount, int32_t& lastFrame,
        float fps, int32_t tileX, int32_t tileY, bool pingpong);

    Renderer& m_Renderer;

    float m_WaterOpacity = 0.95f;
    float m_WaterFps = 12.0f;
    float m_LavaFps = 8.0f;

    std::unique_ptr<Texture2D> m_Texture;
    std::unique_ptr<Texture2D> m_ItemTexture;

    std::array<std::unique_ptr<Pipeline>, k_ChunkPassCount> m_ChunkPipelines;

    std::unique_ptr<Pipeline> m_OutlinePipeline;
    OwnedBuffer m_OutlineVertexBuffer;
    OwnedBuffer m_OutlineIndexBuffer;
    uint32_t m_OutlineIndexCount = 0;

    std::unique_ptr<Pipeline> m_BreakPipeline;
    std::unique_ptr<Texture2D> m_BreakTexture;
    int32_t m_BreakFrameCount = 0;
    OwnedBuffer m_BreakVertexBuffer;
    OwnedBuffer m_BreakIndexBuffer;
    uint32_t m_BreakIndexCount = 0;
    OwnedBuffer m_BreakCrossVertexBuffer;
    OwnedBuffer m_BreakCrossIndexBuffer;
    uint32_t m_BreakCrossIndexCount = 0;

    std::unique_ptr<Pipeline> m_ItemPipeline;
    OwnedBuffer m_ItemVertexBuffer;
    OwnedBuffer m_ItemIndexBuffer;
    uint32_t m_ItemIndexCount = 0;

    std::vector<uint8_t> m_WaterFrames;
    int32_t m_WaterFrameCount = 0;
    int32_t m_WaterFrame = -1;

    std::vector<uint8_t> m_LavaFrames;
    int32_t m_LavaFrameCount = 0;
    int32_t m_LavaFrame = -1;
};

}
