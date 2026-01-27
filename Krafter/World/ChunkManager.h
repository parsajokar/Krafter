#pragma once

#include <deque>
#include <unordered_map>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Layer.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

class ChunkManager : public Layer {
public:
    ChunkManager();
    ~ChunkManager() = default;

    void OnUpdate() override;
    void OnRenderImGui() override;

    inline ChunkMap& GetChunkMap()
    {
        return m_ChunkMap;
    }

    inline const ChunkMap& GetChunkMap() const
    {
        return m_ChunkMap;
    }

private:
    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius);

    ChunkMap m_ChunkMap;

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
