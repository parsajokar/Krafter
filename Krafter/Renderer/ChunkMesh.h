#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Chunk.h"

namespace Krafter {

class ChunkMesh {
public:
    ChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition);
    ~ChunkMesh();

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
