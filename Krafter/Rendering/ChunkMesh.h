#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/Engine/Graphics/GeometryArena.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

enum class ChunkPass {
    k_Opaque,
    k_Cutout,
    k_Translucent,
    k_Transparent,
    k_Count
};

inline constexpr size_t k_ChunkPassCount = static_cast<size_t>(ChunkPass::k_Count);

struct ChunkVertex {
    static constexpr float k_PositionScale = 128.0f;

    uint16_t position[3];
    uint16_t waterDepth;
    uint16_t uv[2];
    int8_t normal[3];
    int8_t blockLight;
    uint8_t tint[3];
    uint8_t skyLight;
};
static_assert(sizeof(ChunkVertex) == 20, "ChunkVertex must match the chunk vertex attributes");

struct ChunkMeshBuffer {
    std::vector<ChunkVertex> vertices;
    std::vector<uint32_t> elements;
};

struct ChunkMeshData {
    std::array<ChunkMeshBuffer, k_ChunkPassCount> passes;

    float minY = 0.0f;
    float maxY = 0.0f;

    bool IsEmpty() const;
};

class Climate;

class ChunkMesh {
public:
    struct Part {
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        int32_t vertexOffset = 0;
    };

    static ChunkMeshData Compute(
        const std::array<const Chunk*, 9>& grid,
        const glm::ivec2& chunkPosition,
        const Climate& climate);

    ChunkMesh(const ChunkMeshData& data, GeometryArena& arena);
    ~ChunkMesh();

    ChunkMesh(const ChunkMesh&) = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;

    const Part& GetPart(ChunkPass pass) const
    {
        return m_Parts[static_cast<size_t>(pass)];
    }

    uint32_t GetPage() const
    {
        return m_Allocation.page;
    }

    bool HasGeometry() const
    {
        return m_Allocation.IsValid();
    }

    float GetMinY() const
    {
        return m_MinY;
    }
    float GetMaxY() const
    {
        return m_MaxY;
    }

private:
    static constexpr float k_NoFlow = 1000.0f;

    static void AddFaceToData(
        const std::array<glm::vec3, 4>& positionList,
        const std::array<glm::vec2, 2>& uvCoordsList,
        const glm::vec3& normal, float waterDepth, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        ChunkMeshBuffer& buffer,
        const std::array<glm::vec2, 4>* explicitUvs = nullptr);
    static void AddFaceToData(
        const glm::vec3& position,
        Block block, BlockFace face, float topInset, float waterDepth, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        ChunkMeshBuffer& buffer,
        const glm::vec2* tileOverride = nullptr, float flowAngle = k_NoFlow,
        const float* cornerHeights = nullptr);
    static void AddOverlayFace(
        const glm::vec3& position, BlockFace face,
        const glm::vec2& tile, const glm::vec3& tint,
        const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
        ChunkMeshBuffer& buffer);

    static void AddCrossToData(
        const glm::vec3& position, const glm::vec2& tile, const glm::vec3& tint, float light, float blockLight,
        ChunkMeshBuffer& buffer,
        const glm::ivec3& growth = glm::ivec3(0, 1, 0));

    std::array<Part, k_ChunkPassCount> m_Parts = {};
    GeometryArena& m_Arena;
    GeometryAllocation m_Allocation;
    float m_MinY = 0.0f;
    float m_MaxY = 0.0f;
};

}
