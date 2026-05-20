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
    static ChunkMeshData Compute(
        const Chunk& center,
        const std::array<const Chunk*, 4>& neighbours,
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
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);
    static void AddFaceToData(
        const glm::vec3& position,
        Block block, BlockFace face,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);

    uint32_t m_ElementCount;

    uint32_t m_VertexArray;
    uint32_t m_VertexBuffer;
    uint32_t m_ElementBuffer;
};

} // namespace Krafter
