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
        return m_chunkMap;
    }

    inline const ChunkMap& GetChunkMap() const
    {
        return m_chunkMap;
    }

private:
    ChunkMap m_chunkMap;

    int32_t m_renderDistance = 10;
    float m_chunkDelay = 0.01f;

    std::deque<glm::ivec2> m_chunkGenerationQueue;
    std::deque<glm::ivec2> m_chunkDeletionQueue;

    std::unordered_map<glm::ivec2, bool> m_onChunkGenerationQueue;
    std::unordered_map<glm::ivec2, bool> m_onChunkDeletionQueue;

    float m_lastChunkGeneration = 0.0f;
    float m_lastChunkDeletion = 0.0f;
};

} // namespace Krafter
