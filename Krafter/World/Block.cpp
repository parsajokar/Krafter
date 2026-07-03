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

    // Column 7: a free tile. Water's pixels come from the animated water_still
    // strip at runtime, so the atlas only needs an unused slot to write into.
    // (Column 6 is the static torch tile, so water's scratch slot sits at 7.)
    s_BlockAtlases[Block::k_Water] = {
        .top = glm::vec2(7.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(7.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(7.0f / 16.0f, 0.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_OakLog] = {
        .top = glm::vec2(0.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 1.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_OakLeaves] = {
        .top = glm::vec2(3.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(3.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 1.0f / 16.0f)
    };

    // Birch and acacia reuse the oak layout one and two rows below it: each tile
    // shares its column with the oak equivalent, just shifted down. Logs are
    // full-colour; the grayscale leaves are biome-tinted like the oak's.
    s_BlockAtlases[Block::k_BirchLog] = {
        .top = glm::vec2(0.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_BirchLeaves] = {
        .top = glm::vec2(3.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(3.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_AcaciaLog] = {
        .top = glm::vec2(0.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 3.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_AcaciaLeaves] = {
        .top = glm::vec2(3.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(3.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 3.0f / 16.0f)
    };

    // The "wood" variants wear each species' bark (its log's side tile) on every
    // face, top and bottom included, so there is no end grain.
    s_BlockAtlases[Block::k_OakWood] = {
        .top = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_BirchWood] = {
        .top = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_AcaciaWood] = {
        .top = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f)
    };

    // Cross plants store their single tile in `side`. Fern and short grass are
    // grayscale and biome-tinted like grass; the dead bush keeps its own brown.
    s_BlockAtlases[Block::k_Fern] = {
        .side = glm::vec2(4.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_ShortGrass] = {
        .side = glm::vec2(5.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_DeadBush] = {
        .side = glm::vec2(6.0f / 16.0f, 2.0f / 16.0f)
    };

    // Cactus is a cube with distinct top, side, and bottom tiles. The mesher
    // insets its side faces so the top and bottom overhang, as in Minecraft.
    s_BlockAtlases[Block::k_Cactus] = {
        .top = glm::vec2(5.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(6.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(7.0f / 16.0f, 1.0f / 16.0f)
    };

    // Planks (crafted from logs) borrow each species' log end-grain tile on every
    // face as a placeholder until they get their own art: oak, birch, and acacia
    // sit in column 0 of their log's row (rows 1, 2, 3).
    s_BlockAtlases[Block::k_OakPlanks] = {
        .top = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_BirchPlanks] = {
        .top = glm::vec2(2.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(2.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_AcaciaPlanks] = {
        .top = glm::vec2(2.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(2.0f / 16.0f, 3.0f / 16.0f)
    };

    // Stone and bedrock, the underground fill and the world floor, each wear one
    // uniform tile on every face.
    s_BlockAtlases[Block::k_Stone] = {
        .top = glm::vec2(14.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(14.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(14.0f / 16.0f, 0.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_Bedrock] = {
        .top = glm::vec2(15.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(15.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(15.0f / 16.0f, 0.0f / 16.0f)
    };

    // Column 5: a free tile the animated lava_still strip is streamed into at
    // runtime, the same way water uses column 7.
    s_BlockAtlases[Block::k_Lava] = {
        .top = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f)
    };

    // Torch is drawn as a cross billboard, which reads only the side tile.
    s_BlockAtlases[Block::k_Torch] = {
        .side = glm::vec2(6.0f / 16.0f, 0.0f / 16.0f)
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
