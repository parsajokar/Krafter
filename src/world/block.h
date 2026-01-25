#pragma once

#include <unordered_map>

#include "glm/glm.hpp"

namespace Krafter {

enum class Block {
    AIR = 0,
    DIRT,
    GRASS
};

enum class BlockFace {
    FRONT,
    BACK,
    LEFT,
    RIGHT,
    BOTTOM,
    TOP
};

class BlockAtlas {
public:
    static void LoadAtlases();
    static const BlockAtlas& GetAtlasOf(Block block);

    static constexpr float STEP = 1.0f / 16.0f;

    glm::vec2 top;
    glm::vec2 side;
    glm::vec2 bottom;

private:
    inline static std::unordered_map<Block, BlockAtlas> _blockAtlases;
};

} // namespace Krafter
