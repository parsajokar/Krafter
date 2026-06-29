#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/Renderer/ShaderProgram.h"
#include "Krafter/Renderer/Texture.h"

namespace Krafter {

class Sky;

// Draws the voxel world: chunk geometry (opaque and transparent water passes),
// the animated water texture, and the targeted-block outline. Owned by the game
// layer, since it only exists while a world is being played.
class WorldRenderer {
public:
    WorldRenderer();
    ~WorldRenderer();

    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;

    void RenderChunkOpaque(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);
    void RenderChunkCross(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);
    void RenderChunkTransparent(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky);
    void SetDepthMask(bool enabled);
    void SetBlend(bool enabled);
    void SetCullFace(bool enabled);

    // Advances the animated water texture, swapping the current frame into the
    // atlas's water tile.
    void AnimateWater();
    void RenderBlockOutline(const glm::ivec3& blockPosition, const glm::mat4& viewProjection);

    // Draws the crack overlay on a block being mined. `progress` is 0..1 through
    // the break and selects which crack stage of the strip to show.
    void RenderBlockBreak(const glm::ivec3& blockPosition, float progress, const glm::mat4& viewProjection);

    // Draws one floating item drop: a flat block-icon billboard centred at
    // `center`, spread across the camera's `right`/`up` axes (each already scaled
    // to the drop's size) and textured from `tileOrigin` (the block's atlas tile
    // origin in 0..1 coords). Sampled from the block atlas, like the HUD icons.
    void RenderItemDrop(
        const glm::vec3& center, const glm::vec3& right, const glm::vec3& up,
        const glm::vec2& tileOrigin, const glm::mat4& viewProjection);

    void RenderImGui();

private:
    void BindChunkProgram(const glm::mat4& viewProjection, const Sky& sky);

    // Opacity of deep water; the shader fades toward clear in the shallows.
    // Tunable live from the ImGui panel.
    float m_WaterOpacity = 0.95f;

    std::unique_ptr<ShaderProgram> m_Program;
    std::unique_ptr<Texture2D> m_Texture;

    std::unique_ptr<ShaderProgram> m_OutlineProgram;
    uint32_t m_OutlineVertexArray;
    uint32_t m_OutlineVertexBuffer;
    uint32_t m_OutlineElementBuffer;
    uint32_t m_OutlineElementCount;

    // Crack overlay: a textured unit cube sampled from the destroy strip (a
    // vertical stack of k_BreakFrameCount progressively-cracked frames).
    std::unique_ptr<ShaderProgram> m_BreakProgram;
    std::unique_ptr<Texture2D> m_BreakTexture;
    int32_t m_BreakFrameCount = 0;
    uint32_t m_BreakVertexArray;
    uint32_t m_BreakVertexBuffer;
    uint32_t m_BreakElementBuffer;
    uint32_t m_BreakElementCount;

    // Item drops: a single unit quad (corner offset + uv) billboarded per draw and
    // textured from the block atlas (m_Texture), so a drop reads as its HUD icon.
    std::unique_ptr<ShaderProgram> m_ItemProgram;
    uint32_t m_ItemVertexArray;
    uint32_t m_ItemVertexBuffer;
    uint32_t m_ItemElementBuffer;
    uint32_t m_ItemElementCount;

    // Animated water: every 16x16 frame of the strip, stored RGBA back-to-back,
    // cycled into the atlas's water tile over time.
    std::vector<uint8_t> m_WaterFrames;
    int32_t m_WaterFrameCount = 0;
    int32_t m_WaterFrame = -1;
};

} // namespace Krafter
