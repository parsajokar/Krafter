#pragma once

#include <unordered_map>

#include "glm/glm.hpp"

namespace Krafter {

enum class Block {
    k_Air = 0,
    k_Dirt,
    k_Grass,
    k_Sand,
    k_Water
};

// Air and water do not hide the faces of blocks behind them, so neighbouring
// solids still mesh their touching faces (you can see the seabed through water).
inline bool IsOpaque(Block block)
{
    return block != Block::k_Air && block != Block::k_Water;
}

enum class BlockFace {
    k_Front,
    k_Back,
    k_Left,
    k_Right,
    k_Bottom,
    k_Top
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
    inline static std::unordered_map<Block, BlockAtlas> s_BlockAtlases;
};

} // namespace Krafter
