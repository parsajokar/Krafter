#include <iostream>
#include <stdexcept>

#include "Krafter/World/Block.h"

namespace Krafter {

void BlockAtlas::LoadAtlases()
{
    s_BlockAtlases[Block::k_Dirt] = {
        .top = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_Grass] = {
        .top = glm::vec2(2.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f)
    };
}

const BlockAtlas& BlockAtlas::GetAtlasOf(Block block)
{
    try {
        return s_BlockAtlases.at(block);
    } catch (std::out_of_range e) {
        std::cerr << "Block atlas is not defined!" << std::endl;
        throw e;
    }
}

} // namespace Krafter
