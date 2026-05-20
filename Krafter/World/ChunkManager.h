#pragma once

#include <deque>
#include <unordered_map>

#include "glm/glm.hpp"

#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

class ChunkManager {
public:
    void Update();
    void Render();
    void RenderImGui();

private:
    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius);

    void LoadChunk(const glm::ivec2& chunkPosition);
    void UnloadChunk(const glm::ivec2& chunkPosition);

    bool IsChunkLoadedToChunkMap(const glm::ivec2& chunkPosition) const;
    void LoadChunkToChunkMap(const glm::ivec2& chunkPosition);

    ChunkMap m_ChunkMap;
    ChunkMeshMap m_ChunkMeshMap;

    int32_t m_RenderDistance = 10;
    float m_ChunkDelay = 0.01f;

    std::deque<glm::ivec2> m_ChunkGenerationQueue;
    std::deque<glm::ivec2> m_ChunkDeletionQueue;

    std::unordered_map<glm::ivec2, bool> m_OnChunkGenerationQueue;
    std::unordered_map<glm::ivec2, bool> m_OnChunkDeletionQueue;

    float m_LastChunkGeneration = 0.0f;
    float m_LastChunkDeletion = 0.0f;
};

} // namespace Krafter
