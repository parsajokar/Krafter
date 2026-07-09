#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Core/Renderer.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

// Interleaved vertex data plus indices for one render pass.
struct ChunkMeshBuffer {
    std::vector<float> vertices;
    std::vector<uint32_t> elements;
};

// Opaque geometry is drawn first; cross-shaped plants follow in a cutout pass
// (double-sided, depth-writing); transparent water is last so it blends over
// what is behind it.
struct ChunkMeshData {
    ChunkMeshBuffer opaque;
    ChunkMeshBuffer cross;
    ChunkMeshBuffer transparent;
};

class ChunkMesh {
public:
    // `grid` is the 3x3 chunk neighbourhood, indexed by (dz + 1) * 3 + (dx + 1)
    // (centre at index 4), so smooth lighting and AO can sample diagonal cells
    // across chunk borders.
    static ChunkMeshData Compute(
        const std::array<const Chunk*, 9>& grid,
        const glm::ivec2& chunkPosition);

    explicit ChunkMesh(const ChunkMeshData& data);
    ~ChunkMesh();

    ChunkMesh(const ChunkMesh&) = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;

    inline uint32_t GetOpaqueElementCount() const
    {
        return m_Opaque.elementCount;
    }

    inline uint32_t GetCrossElementCount() const
    {
        return m_Cross.elementCount;
    }

    inline uint32_t GetTransparentElementCount() const
    {
        return m_Transparent.elementCount;
    }

    // Records the vertex/index buffer binds and an indexed draw for this part into
    // `cmd`. The caller binds the pipeline, descriptor set, and push constants first.
    void DrawOpaque(VkCommandBuffer cmd) const;
    void DrawCross(VkCommandBuffer cmd) const;
    void DrawTransparent(VkCommandBuffer cmd) const;

private:
    // One set of GPU buffers for a single render pass.
    struct Part {
        GpuBuffer vertexBuffer;
        GpuBuffer indexBuffer;
        uint32_t elementCount = 0;
    };

    static void Upload(Part& part, const ChunkMeshBuffer& buffer);
    static void Release(Part& part);

    static void AddFaceToData(
        const std::array<glm::vec3, 4>& positionList,
        const std::array<glm::vec2, 2>& uvCoordsList,
        const glm::vec3& normal, float waterDepth, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);
    static void AddFaceToData(
        const glm::vec3& position,
        Block block, BlockFace face, float topInset, float waterDepth, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);
    // The biome-tinted grass fringe, drawn over a grass side face and nudged
    // outward so it does not z-fight the dirt base beneath it.
    static void AddOverlayFace(
        const glm::vec3& position, BlockFace face,
        const glm::vec2& tile, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);

    // Two crossed billboards filling the cell corner-to-corner, used for plants.
    // Lit flatly from the cell's own sky and block light and drawn double-sided.
    static void AddCrossToData(
        const glm::vec3& position, const glm::vec2& tile, const glm::vec3& tint, float light, float blockLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);

    Part m_Opaque;
    Part m_Cross;
    Part m_Transparent;
};

} // namespace Krafter
