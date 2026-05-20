#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

class World {
public:
    void Update();
    void Render();
    void RenderImGui();

    Block GetBlock(const glm::ivec3& worldPosition) const;

private:
    enum class ChunkState {
        k_TerrainReady,
        k_MeshReady
    };

    struct ChunkRecord {
        Chunk chunk;
        std::unique_ptr<ChunkMesh> mesh;
        ChunkState state;

        explicit ChunkRecord(const glm::ivec2& position)
            : chunk(position)
            , state(ChunkState::k_TerrainReady)
        {
        }
    };

    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius);

    void GenerateTerrain(const glm::ivec2& chunkPosition);
    void GenerateMesh(const glm::ivec2& chunkPosition);
    void Unload(const glm::ivec2& chunkPosition);

    bool HasTerrainNeighbours(const glm::ivec2& chunkPosition) const;

    std::unordered_map<glm::ivec2, ChunkRecord> m_Chunks;

    int32_t m_RenderDistance = 10;
    float m_ChunkDelay = 0.01f;

    std::deque<glm::ivec2> m_TerrainQueue;
    std::deque<glm::ivec2> m_MeshQueue;
    std::deque<glm::ivec2> m_UnloadQueue;

    std::unordered_set<glm::ivec2> m_QueuedTerrain;
    std::unordered_set<glm::ivec2> m_QueuedMesh;
    std::unordered_set<glm::ivec2> m_QueuedUnload;

    float m_LastChunkUpdate = 0.0f;
};

} // namespace Krafter
