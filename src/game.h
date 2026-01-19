#pragma once

#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "block.h"

namespace Krafter
{

class Game
{
public:
    static void Init();
    static void Deinit();
    inline static Game* Get() { return _instance; }

    void Run();
    inline float GetDelta() const { return _delta; };

private:
    inline static Game* _instance;

    Game();
    ~Game();

    float _delta;
    ChunkMap _chunkMap;
};

} // namespace Krafter