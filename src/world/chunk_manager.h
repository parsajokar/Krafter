#pragma once

#include <deque>
#include <unordered_map>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "layer.h"
#include "world/chunk.h"

namespace Krafter {

class ChunkManager : public Layer {
public:
    ChunkManager();
    ~ChunkManager() = default;

    void OnUpdate() override;
    void OnRenderImGui() override;

    inline ChunkMap& GetChunkMap()
    {
        return _chunkMap;
    }

    inline const ChunkMap& GetChunkMap() const
    {
        return _chunkMap;
    }

private:
    ChunkMap _chunkMap;

    int32_t _renderDistance = 10;
    float _chunkDelay = 0.01f;

    std::deque<glm::ivec2> _chunkGenerationQueue;
    std::deque<glm::ivec2> _chunkDeletionQueue;

    std::unordered_map<glm::ivec2, bool> _onChunkGenerationQueue;
    std::unordered_map<glm::ivec2, bool> _onChunkDeletionQueue;

    float _lastChunkGeneration = 0.0f;
    float _lastChunkDeletion = 0.0f;
};

} // namespace Krafter
