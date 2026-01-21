#pragma once

#include <queue>
#include <unordered_set>

#include "block.h"

namespace Krafter
{

class Game
{
public:
    static void Init();
    static void Deinit();
    inline static Game* Get()
    {
        return _instance;
    }

    void Run();

    inline float GetDelta() const
    {
        return _delta;
    }

private:
    inline static Game* _instance;

    Game();
    ~Game();

    float _delta;
    float _fps;

    ChunkMap _chunkMap;
    std::queue<glm::ivec2> _chunkGenerationQueue;
    std::queue<glm::ivec2> _chunkDeletionQueue;
    std::unordered_set<glm::ivec2> _onChunkGenerationQueue;
    std::unordered_set<glm::ivec2> _onChunkDeletionQueue;
};

} // namespace Krafter
