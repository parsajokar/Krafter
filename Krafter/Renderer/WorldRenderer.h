#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/Renderer/Pipeline.h"
#include "Krafter/Renderer/Texture.h"

namespace Krafter {

class Sky;

// Draws the voxel world: chunk geometry (opaque, cross-plant, and transparent
// water passes), the animated water and lava textures, and the targeted-block
// outline. Owned by the game layer, since it only exists while a world is played.
class WorldRenderer {
public:
    WorldRenderer();
    ~WorldRenderer();

    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;

    void RenderChunkOpaque(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);
    void RenderChunkCross(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);
    void RenderChunkTransparent(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);

    // The old GL state toggles are now baked into each pass's pipeline, so these
    // are kept only for World::Render's call sites and do nothing.
    void SetDepthMask(bool enabled) { }
    void SetBlend(bool enabled) { }
    void SetCullFace(bool enabled) { }

    // Advances the animated water texture, swapping the current frame into the
    // atlas's water tile.
    void AnimateWater();

    // Same for the animated lava texture, cycled into the atlas's lava tile.
    void AnimateLava();

    void RenderBlockOutline(const glm::ivec3& blockPosition, const glm::mat4& viewProjection);

    // Draws the crack overlay on a block being mined. `progress` is 0..1 through
    // the break and selects which crack stage of the strip to show.
    void RenderBlockBreak(const glm::ivec3& blockPosition, float progress, const glm::mat4& viewProjection);

    // Draws one floating item drop: a flat block-icon billboard centred at
    // `center`, spread across the camera's `right`/`up` axes (each already scaled
    // to the drop's size) and textured from `tileOrigin`.
    void RenderItemDrop(
        const glm::vec3& center, const glm::vec3& right, const glm::vec3& up,
        const glm::vec2& tileOrigin, const glm::mat4& viewProjection);

    void RenderImGui();

private:
    // Per-draw constants for the chunk shaders. The field order places each float
    // in the padding a std430 vec3 leaves behind, so the offsets match the shader's
    // push_constant block exactly (checked by the static_assert in the .cpp).
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

    // outline.vert: view-projection and the targeted block's corner.
    struct OutlinePush {
        glm::mat4 viewProjection;
        glm::vec3 blockPosition;
        float pad;
    };
    static_assert(sizeof(OutlinePush) == 80, "OutlinePush must match outline.vert");

    // break.vert: the block corner plus the crack strip's active frame band.
    struct BreakPush {
        glm::mat4 viewProjection;
        glm::vec3 blockPosition;
        float frameBase;
        float frameSpan;
    };
    static_assert(sizeof(BreakPush) == 84, "BreakPush must match break.vert");

    // item.vert: the billboard centre, its camera-aligned axes, and the atlas tile.
    // Each vec3 is padded to 16 bytes to match the shader's std430 layout.
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

    // Opacity of deep water; the shader fades toward clear in the shallows.
    float m_WaterOpacity = 0.95f;

    std::unique_ptr<Texture2D> m_Texture;

    // One pipeline per chunk pass: opaque (cull back, depth write), cross plants
    // (double-sided, depth write), and water (double-sided, no depth write).
    std::unique_ptr<Pipeline> m_OpaquePipeline;
    std::unique_ptr<Pipeline> m_CrossPipeline;
    std::unique_ptr<Pipeline> m_TransparentPipeline;

    // Targeted-block wireframe: a unit-cube outline drawn with line topology.
    std::unique_ptr<Pipeline> m_OutlinePipeline;
    GpuBuffer m_OutlineVertexBuffer;
    GpuBuffer m_OutlineIndexBuffer;
    uint32_t m_OutlineIndexCount = 0;

    // Crack overlay: a unit cube whose faces each carry the whole crack tile,
    // sampled from the destroy strip and depth-biased onto the block face.
    std::unique_ptr<Pipeline> m_BreakPipeline;
    std::unique_ptr<Texture2D> m_BreakTexture;
    int32_t m_BreakFrameCount = 0;
    GpuBuffer m_BreakVertexBuffer;
    GpuBuffer m_BreakIndexBuffer;
    uint32_t m_BreakIndexCount = 0;

    // Item drops: a single billboarded quad, textured from the block atlas.
    std::unique_ptr<Pipeline> m_ItemPipeline;
    GpuBuffer m_ItemVertexBuffer;
    GpuBuffer m_ItemIndexBuffer;
    uint32_t m_ItemIndexCount = 0;

    // Animated water: every 16x16 frame of the strip, stored RGBA back-to-back,
    // cycled into the atlas's water tile over time.
    std::vector<uint8_t> m_WaterFrames;
    int32_t m_WaterFrameCount = 0;
    int32_t m_WaterFrame = -1;

    // Animated lava, streamed into the atlas's lava tile the same way.
    std::vector<uint8_t> m_LavaFrames;
    int32_t m_LavaFrameCount = 0;
    int32_t m_LavaFrame = -1;
};

} // namespace Krafter
