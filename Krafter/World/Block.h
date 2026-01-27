#pragma once

#include <unordered_map>

#include "glm/glm.hpp"

namespace Krafter {

enum class Block {
    Air = 0,
    Dirt,
    Grass
};

enum class BlockFace {
    Front,
    Back,
    Left,
    Right,
    Bottom,
    Top
};

class BlockAtlas {
public:
    static void LoadAtlases();
    static const BlockAtlas& GetAtlasOf(Block block);

    static constexpr float k_Step = 1.0f / 16.0f;

    glm::vec2 top;
    glm::vec2 side;
    glm::vec2 bottom;

private:
    inline static std::unordered_map<Block, BlockAtlas> s_blockAtlases;
};

} // namespace Krafter
