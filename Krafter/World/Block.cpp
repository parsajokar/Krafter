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

    s_BlockAtlases[Block::k_Fern] = {
        .side = glm::vec2(4.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_ShortGrass] = {
        .side = glm::vec2(5.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_DeadBush] = {
        .side = glm::vec2(6.0f / 16.0f, 2.0f / 16.0f)
    };

    s_BlockAtlases[Block::k_Cactus] = {
        .top = glm::vec2(5.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(6.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(7.0f / 16.0f, 1.0f / 16.0f)
    };

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

    s_BlockAtlases[Block::k_Lava] = {
        .top = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f)
    };

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

}
