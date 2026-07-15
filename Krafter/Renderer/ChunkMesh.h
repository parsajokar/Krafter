#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Core/Renderer.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

struct ChunkMeshBuffer {
    std::vector<float> vertices;
    std::vector<uint32_t> elements;
};

struct ChunkMeshData {
    ChunkMeshBuffer opaque;
    ChunkMeshBuffer cross;
    ChunkMeshBuffer transparent;
};

class ChunkMesh {
public:
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

    void DrawOpaque(VkCommandBuffer cmd) const;
    void DrawCross(VkCommandBuffer cmd) const;
    void DrawTransparent(VkCommandBuffer cmd) const;

private:
    static constexpr float k_NoFlow = 1000.0f;

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
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData,
        const std::array<glm::vec2, 4>* explicitUvs = nullptr);
    static void AddFaceToData(
        const glm::vec3& position,
        Block block, BlockFace face, float topInset, float waterDepth, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData,
        const glm::vec2* tileOverride = nullptr, float flowAngle = k_NoFlow,
        const float* cornerHeights = nullptr);
    static void AddOverlayFace(
        const glm::vec3& position, BlockFace face,
        const glm::vec2& tile, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);

    static void AddCrossToData(
        const glm::vec3& position, const glm::vec2& tile, const glm::vec3& tint, float light, float blockLight,
        std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData);

    Part m_Opaque;
    Part m_Cross;
    Part m_Transparent;
};

}
