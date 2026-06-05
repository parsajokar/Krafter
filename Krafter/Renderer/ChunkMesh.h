#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

struct ChunkMeshData {
    std::vector<float> vertices;
    std::vector<uint32_t> elements;
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

    inline uint32_t GetElementCount() const
    {
        return m_ElementCount;
    }

    void Bind() const;

private:
    static void AddFaceToData(
        const std::array<glm::vec3, 4>& positionList,
        const std::array<glm::vec2, 2>& uvCoordsList,
        const glm::vec3& normal,
        const std::array<float, 4>& vertexLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);
    static void AddFaceToData(
        const glm::vec3& position,
        Block block, BlockFace face,
        const std::array<float, 4>& vertexLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);

    uint32_t m_ElementCount;

    uint32_t m_VertexArray;
    uint32_t m_VertexBuffer;
    uint32_t m_ElementBuffer;
};

} // namespace Krafter
