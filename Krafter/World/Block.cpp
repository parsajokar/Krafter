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

    // Grass top and side overlay are grayscale; the mesher tints them with the
    // biome's grass colour. The side base and bottom are the plain dirt browns.
    s_BlockAtlases[Block::k_Grass] = {
        .top = glm::vec2(3.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .sideOverlay = glm::vec2(2.0f / 16.0f, 0.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_Sand] = {
        .top = glm::vec2(4.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(4.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(4.0f / 16.0f, 0.0f / 16.0f)
    };

    // Column 6: a free tile. Water's pixels come from the animated water_still
    // strip at runtime, so the atlas only needs an unused slot to write into.
    s_BlockAtlases[Block::k_Water] = {
        .top = glm::vec2(6.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(6.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(6.0f / 16.0f, 0.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_OakLog] = {
        .top = glm::vec2(0.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 1.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_OakLeaves] = {
        .top = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f)
    };

    // Cross plants store their single tile in `side`. Fern and short grass are
    // grayscale and biome-tinted like grass; the dead bush keeps its own brown.
    s_BlockAtlases[Block::k_Fern] = {
        .side = glm::vec2(3.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_ShortGrass] = {
        .side = glm::vec2(4.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_DeadBush] = {
        .side = glm::vec2(5.0f / 16.0f, 2.0f / 16.0f)
    };

    // Cactus is a cube with distinct top, side, and bottom tiles. The mesher
    // insets its side faces so the top and bottom overhang, as in Minecraft.
    s_BlockAtlases[Block::k_Cactus] = {
        .top = glm::vec2(4.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(5.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 1.0f / 16.0f)
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
