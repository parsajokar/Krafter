#include <array>
#include <cstddef>

#include "Krafter/World/Block.h"

namespace Krafter {

namespace {

constexpr std::array<BlockAtlas, static_cast<size_t>(Block::k_Count)> k_BlockAtlases = [] {
    std::array<BlockAtlas, static_cast<size_t>(Block::k_Count)> atlases = {};

    atlases[static_cast<size_t>(Block::k_Dirt)] = BlockAtlas {
        .top = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Grass)] = BlockAtlas {
        .top = glm::vec2(3.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .sideOverlay = glm::vec2(2.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Sand)] = BlockAtlas {
        .top = glm::vec2(4.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(4.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(4.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Water)] = BlockAtlas {
        .top = glm::vec2(7.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(7.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(7.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_OakLog)] = BlockAtlas {
        .top = glm::vec2(0.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_OakLeaves)] = BlockAtlas {
        .top = glm::vec2(3.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(3.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_BirchLog)] = BlockAtlas {
        .top = glm::vec2(0.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_BirchLeaves)] = BlockAtlas {
        .top = glm::vec2(3.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(3.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_AcaciaLog)] = BlockAtlas {
        .top = glm::vec2(0.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 3.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_AcaciaLeaves)] = BlockAtlas {
        .top = glm::vec2(3.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(3.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 3.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_OakWood)] = BlockAtlas {
        .top = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(1.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_BirchWood)] = BlockAtlas {
        .top = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(1.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_AcaciaWood)] = BlockAtlas {
        .top = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(1.0f / 16.0f, 3.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Fern)] = BlockAtlas {
        .side = glm::vec2(4.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_ShortGrass)] = BlockAtlas {
        .side = glm::vec2(5.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_DeadBush)] = BlockAtlas {
        .side = glm::vec2(6.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_CactusFlower)] = BlockAtlas {
        .side = glm::vec2(7.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Poppy)] = BlockAtlas {
        .side = glm::vec2(4.0f / 16.0f, 3.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Dandelion)] = BlockAtlas {
        .side = glm::vec2(5.0f / 16.0f, 3.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Allium)] = BlockAtlas {
        .side = glm::vec2(6.0f / 16.0f, 3.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Cactus)] = BlockAtlas {
        .top = glm::vec2(5.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(6.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(5.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_OakPlanks)] = BlockAtlas {
        .top = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f),
        .bottom = glm::vec2(2.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_BirchPlanks)] = BlockAtlas {
        .top = glm::vec2(2.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(2.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_AcaciaPlanks)] = BlockAtlas {
        .top = glm::vec2(2.0f / 16.0f, 3.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 3.0f / 16.0f),
        .bottom = glm::vec2(2.0f / 16.0f, 3.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Stone)] = BlockAtlas {
        .top = glm::vec2(14.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(14.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(14.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Bedrock)] = BlockAtlas {
        .top = glm::vec2(15.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(15.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(15.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Lava)] = BlockAtlas {
        .top = glm::vec2(8.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(8.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(8.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_RedSand)] = BlockAtlas {
        .top = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(5.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Torch)] = BlockAtlas {
        .side = glm::vec2(6.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_IronOre)] = BlockAtlas {
        .top = glm::vec2(11.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(11.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(11.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_CopperOre)] = BlockAtlas {
        .top = glm::vec2(12.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(12.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(12.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_CoalOre)] = BlockAtlas {
        .top = glm::vec2(13.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(13.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(13.0f / 16.0f, 0.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Topaz)] = BlockAtlas {
        .side = glm::vec2(12.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Emerald)] = BlockAtlas {
        .side = glm::vec2(13.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Amethyst)] = BlockAtlas {
        .side = glm::vec2(14.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Diamond)] = BlockAtlas {
        .side = glm::vec2(15.0f / 16.0f, 1.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_HardIce)] = BlockAtlas {
        .top = glm::vec2(15.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(15.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(15.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Ice)] = BlockAtlas {
        .top = glm::vec2(14.0f / 16.0f, 2.0f / 16.0f),
        .side = glm::vec2(14.0f / 16.0f, 2.0f / 16.0f),
        .bottom = glm::vec2(14.0f / 16.0f, 2.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Workbench)] = BlockAtlas {
        .top = glm::vec2(0.0f / 16.0f, 15.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 15.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 15.0f / 16.0f),
        .front = glm::vec2(1.0f / 16.0f, 15.0f / 16.0f)
    };

    atlases[static_cast<size_t>(Block::k_Furnace)] = BlockAtlas {
        .top = glm::vec2(0.0f / 16.0f, 14.0f / 16.0f),
        .side = glm::vec2(2.0f / 16.0f, 14.0f / 16.0f),
        .bottom = glm::vec2(3.0f / 16.0f, 14.0f / 16.0f),
        .front = glm::vec2(1.0f / 16.0f, 14.0f / 16.0f)
    };

    return atlases;
}();

}

const BlockAtlas& BlockAtlas::GetAtlasOf(Block block)
{
    return k_BlockAtlases[static_cast<size_t>(block)];
}

}
