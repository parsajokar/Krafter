#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "glm/glm.hpp"

namespace Krafter {

enum class Block {
    k_Air = 0,
    k_Dirt,
    k_Grass,
    k_Sand,
    k_Water,
    k_OakLog,
    k_OakLeaves,
    k_BirchLog,
    k_BirchLeaves,
    k_AcaciaLog,
    k_AcaciaLeaves,
    k_OakWood,
    k_BirchWood,
    k_AcaciaWood,
    k_ShortGrass,
    k_Fern,
    k_DeadBush,
    k_Cactus,
    k_OakPlanks,
    k_BirchPlanks,
    k_AcaciaPlanks,
    k_Stone,
    k_Bedrock,
    k_Lava,
    k_Torch,
    k_IronOre,
    k_CopperOre,
    k_CoalOre,
    k_Topaz,
    k_Emerald,
    k_Amethyst,
    k_Diamond,
    k_HardIce,
    k_Ice,
    k_CactusFlower,

    k_Count // must stay last: sizes k_BlockInfo and counts the kinds above
};

enum class ToolType : uint32_t {
    k_None = 0,
    k_Axe = 1 << 0,
    k_Pickaxe = 1 << 1,
    k_Shovel = 1 << 2,
};

inline constexpr ToolType operator|(ToolType a, ToolType b)
{
    return static_cast<ToolType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr ToolType operator&(ToolType a, ToolType b)
{
    return static_cast<ToolType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline constexpr bool HasTool(ToolType set, ToolType tool)
{
    return tool != ToolType::k_None && (set & tool) != ToolType::k_None;
}

enum class BlockCategory {
    k_None,
    k_Log,
    k_Wood,
    k_Planks,
    k_Leaves,
    k_Plant,
    k_Gem,
};

struct BlockInfo {
    Block id = Block::k_Air;

    bool opaque = false;
    bool cutout = false;
    // Solid like an opaque block for gameplay, but rendered with blending so you
    // can see through it (e.g. ice). Only affects meshing/rendering.
    bool translucent = false;

    BlockCategory category = BlockCategory::k_None;

    float breakSeconds = 0.75f;
    Block drop = Block::k_Air;
    ToolType harvest = ToolType::k_None;

    uint8_t emission = 0;
};

inline constexpr std::array<BlockInfo, static_cast<size_t>(Block::k_Count)> k_BlockInfo = { {
    { .id = Block::k_Air },
    { .id = Block::k_Dirt, .opaque = true, .drop = Block::k_Dirt, .harvest = ToolType::k_Shovel },
    { .id = Block::k_Grass, .opaque = true, .drop = Block::k_Dirt, .harvest = ToolType::k_Shovel },
    { .id = Block::k_Sand, .opaque = true, .drop = Block::k_Sand, .harvest = ToolType::k_Shovel },
    { .id = Block::k_Water },
    { .id = Block::k_OakLog, .opaque = true, .category = BlockCategory::k_Log, .breakSeconds = 1.2f, .drop = Block::k_OakLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_OakLeaves, .opaque = true, .cutout = true, .category = BlockCategory::k_Leaves, .breakSeconds = 0.3f, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchLog, .opaque = true, .category = BlockCategory::k_Log, .breakSeconds = 1.2f, .drop = Block::k_BirchLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchLeaves, .opaque = true, .cutout = true, .category = BlockCategory::k_Leaves, .breakSeconds = 0.3f, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaLog, .opaque = true, .category = BlockCategory::k_Log, .breakSeconds = 1.2f, .drop = Block::k_AcaciaLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaLeaves, .opaque = true, .cutout = true, .category = BlockCategory::k_Leaves, .breakSeconds = 0.3f, .harvest = ToolType::k_Axe },
    { .id = Block::k_OakWood, .opaque = true, .category = BlockCategory::k_Wood, .breakSeconds = 1.2f, .drop = Block::k_OakLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchWood, .opaque = true, .category = BlockCategory::k_Wood, .breakSeconds = 1.2f, .drop = Block::k_BirchLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaWood, .opaque = true, .category = BlockCategory::k_Wood, .breakSeconds = 1.2f, .drop = Block::k_AcaciaLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_ShortGrass, .category = BlockCategory::k_Plant, .breakSeconds = 0.0f, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel },
    { .id = Block::k_Fern, .category = BlockCategory::k_Plant, .breakSeconds = 0.0f, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel },
    { .id = Block::k_DeadBush, .category = BlockCategory::k_Plant, .breakSeconds = 0.0f, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel },
    { .id = Block::k_Cactus, .opaque = true, .cutout = true, .breakSeconds = 0.5f, .drop = Block::k_Cactus, .harvest = ToolType::k_Axe },
    { .id = Block::k_OakPlanks, .opaque = true, .category = BlockCategory::k_Planks, .breakSeconds = 1.2f, .drop = Block::k_OakPlanks, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchPlanks, .opaque = true, .category = BlockCategory::k_Planks, .breakSeconds = 1.2f, .drop = Block::k_BirchPlanks, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaPlanks, .opaque = true, .category = BlockCategory::k_Planks, .breakSeconds = 1.2f, .drop = Block::k_AcaciaPlanks, .harvest = ToolType::k_Axe },
    { .id = Block::k_Stone, .opaque = true, .breakSeconds = 1.5f, .drop = Block::k_Stone, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_Bedrock, .opaque = true, .harvest = ToolType::k_None },
    { .id = Block::k_Lava, .harvest = ToolType::k_None, .emission = 15 },
    { .id = Block::k_Torch, .category = BlockCategory::k_Plant, .breakSeconds = 0.0f, .drop = Block::k_Torch, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel, .emission = 14 },
    { .id = Block::k_IronOre, .opaque = true, .breakSeconds = 2.2f, .drop = Block::k_IronOre, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_CopperOre, .opaque = true, .breakSeconds = 2.0f, .drop = Block::k_CopperOre, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_CoalOre, .opaque = true, .breakSeconds = 1.8f, .drop = Block::k_CoalOre, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_Topaz, .category = BlockCategory::k_Gem, .breakSeconds = 0.6f, .drop = Block::k_Topaz, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_Emerald, .category = BlockCategory::k_Gem, .breakSeconds = 0.6f, .drop = Block::k_Emerald, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_Amethyst, .category = BlockCategory::k_Gem, .breakSeconds = 0.6f, .drop = Block::k_Amethyst, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_Diamond, .category = BlockCategory::k_Gem, .breakSeconds = 0.6f, .drop = Block::k_Diamond, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_HardIce, .opaque = true, .breakSeconds = 0.5f, .drop = Block::k_HardIce, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_Ice, .opaque = true, .translucent = true, .breakSeconds = 0.4f, .drop = Block::k_Ice, .harvest = ToolType::k_Pickaxe },
    { .id = Block::k_CactusFlower, .category = BlockCategory::k_Plant, .breakSeconds = 0.0f, .drop = Block::k_CactusFlower, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel },
} };

constexpr bool BlockInfoTableInOrder()
{
    for (size_t i = 0; i < k_BlockInfo.size(); ++i) {
        if (static_cast<size_t>(k_BlockInfo[i].id) != i) {
            return false;
        }
    }
    return true;
}
static_assert(BlockInfoTableInOrder(), "k_BlockInfo rows must follow the Block enum order");

inline constexpr const BlockInfo& BlockInfoOf(Block block)
{
    return k_BlockInfo[static_cast<size_t>(block)];
}

inline constexpr bool IsLog(Block block) { return BlockInfoOf(block).category == BlockCategory::k_Log; }
inline constexpr bool IsWood(Block block) { return BlockInfoOf(block).category == BlockCategory::k_Wood; }
inline constexpr bool IsPlanks(Block block) { return BlockInfoOf(block).category == BlockCategory::k_Planks; }
inline constexpr bool IsLeaves(Block block) { return BlockInfoOf(block).category == BlockCategory::k_Leaves; }

inline constexpr bool IsPlant(Block block) { return BlockInfoOf(block).category == BlockCategory::k_Plant; }

inline constexpr bool IsGem(Block block) { return BlockInfoOf(block).category == BlockCategory::k_Gem; }

inline constexpr bool IsOre(Block block)
{
    return block == Block::k_IronOre || block == Block::k_CopperOre || block == Block::k_CoalOre;
}

inline constexpr bool IsTreePart(Block block)
{
    const BlockCategory category = BlockInfoOf(block).category;
    return category == BlockCategory::k_Log || category == BlockCategory::k_Wood
        || category == BlockCategory::k_Leaves;
}

inline constexpr bool IsNaturalTreePart(Block block)
{
    const BlockCategory category = BlockInfoOf(block).category;
    return category == BlockCategory::k_Wood || category == BlockCategory::k_Leaves;
}

inline constexpr bool IsOpaque(Block block) { return BlockInfoOf(block).opaque; }

inline constexpr bool IsCutout(Block block) { return BlockInfoOf(block).cutout; }

inline constexpr bool IsTranslucent(Block block) { return BlockInfoOf(block).translucent; }

inline constexpr bool IsTargetable(Block block)
{
    return IsOpaque(block) || IsPlant(block) || IsGem(block);
}

inline constexpr float BreakSeconds(Block block) { return BlockInfoOf(block).breakSeconds; }

inline constexpr Block DropFor(Block block) { return BlockInfoOf(block).drop; }

inline constexpr ToolType HarvestTools(Block block) { return BlockInfoOf(block).harvest; }

inline constexpr uint8_t LightEmission(Block block) { return BlockInfoOf(block).emission; }

inline constexpr bool CanHarvestWith(Block block, ToolType tool)
{
    return HasTool(HarvestTools(block), tool);
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
    glm::vec2 sideOverlay;

private:
    inline static std::unordered_map<Block, BlockAtlas> s_BlockAtlases;
};

inline glm::vec2 BlockIconTile(Block block)
{
    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);
    return IsLog(block) ? atlas.top : atlas.side;
}

}
