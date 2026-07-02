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

    // Number of block kinds; must stay last. Sizes the per-block data table.
    k_Count
};

// The kinds of tool that can harvest a block, as bit flags so a block can be
// tagged with several (e.g. an axe-or-pickaxe block) and a tool's kind matched
// against them. k_None means no tool harvests it yet (or, for a tool, that it
// harvests nothing). Combine with `|`; test with `HasTool`.
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

// Whether `set` contains `tool` (any overlapping bit). k_None never matches, so a
// toolless block or a capability-less tool harvests nothing.
inline constexpr bool HasTool(ToolType set, ToolType tool)
{
    return tool != ToolType::k_None && (set & tool) != ToolType::k_None;
}

// One block's full profile: how it renders and occludes, its tree/material
// groupings, and its gameplay data (mining time, drop, harvesting tools). This is
// the single place a block's properties live; every predicate below is a thin
// read of it, so adding a block is one row in k_BlockInfo, not edits scattered
// across a dozen functions.
struct BlockInfo {
    Block id = Block::k_Air;

    bool opaque = false; // hides neighbouring faces and fully blocks light
    bool cutout = false; // see-through texels (still opaque for targeting/light)
    bool plant = false; // cross-shaped billboard: no collision, lets light pass

    // Material groupings, used by tree physics, drops, and flat icons.
    bool log = false;
    bool wood = false;
    bool planks = false;
    bool leaves = false;

    float breakSeconds = 0.75f; // time to mine with the proper tool
    Block drop = Block::k_Air; // what it yields when broken (k_Air: none)
    ToolType harvest = ToolType::k_None; // tools that can break it (mix with `|`)
};

// Every block's profile, indexed by the Block enum's value, so the rows must stay
// in enum order (checked just below). Omitted fields take the struct's defaults.
inline constexpr std::array<BlockInfo, static_cast<size_t>(Block::k_Count)> k_BlockInfo = { {
    { .id = Block::k_Air },
    { .id = Block::k_Dirt, .opaque = true, .drop = Block::k_Dirt, .harvest = ToolType::k_Shovel },
    { .id = Block::k_Grass, .opaque = true, .drop = Block::k_Dirt, .harvest = ToolType::k_Shovel }, // digging turf yields plain dirt
    { .id = Block::k_Sand, .opaque = true, .drop = Block::k_Sand, .harvest = ToolType::k_Shovel },
    { .id = Block::k_Water },
    { .id = Block::k_OakLog, .opaque = true, .log = true, .breakSeconds = 1.2f, .drop = Block::k_OakLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_OakLeaves, .opaque = true, .cutout = true, .leaves = true, .breakSeconds = 0.3f, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchLog, .opaque = true, .log = true, .breakSeconds = 1.2f, .drop = Block::k_BirchLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchLeaves, .opaque = true, .cutout = true, .leaves = true, .breakSeconds = 0.3f, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaLog, .opaque = true, .log = true, .breakSeconds = 1.2f, .drop = Block::k_AcaciaLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaLeaves, .opaque = true, .cutout = true, .leaves = true, .breakSeconds = 0.3f, .harvest = ToolType::k_Axe },
    { .id = Block::k_OakWood, .opaque = true, .wood = true, .breakSeconds = 1.2f, .drop = Block::k_OakLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchWood, .opaque = true, .wood = true, .breakSeconds = 1.2f, .drop = Block::k_BirchLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaWood, .opaque = true, .wood = true, .breakSeconds = 1.2f, .drop = Block::k_AcaciaLog, .harvest = ToolType::k_Axe },
    { .id = Block::k_ShortGrass, .plant = true, .breakSeconds = 0.0f, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel },
    { .id = Block::k_Fern, .plant = true, .breakSeconds = 0.0f, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel },
    { .id = Block::k_DeadBush, .plant = true, .breakSeconds = 0.0f, .harvest = ToolType::k_Axe | ToolType::k_Pickaxe | ToolType::k_Shovel },
    { .id = Block::k_Cactus, .opaque = true, .cutout = true, .breakSeconds = 0.5f, .drop = Block::k_Cactus, .harvest = ToolType::k_Axe },
    { .id = Block::k_OakPlanks, .opaque = true, .planks = true, .breakSeconds = 1.2f, .drop = Block::k_OakPlanks, .harvest = ToolType::k_Axe },
    { .id = Block::k_BirchPlanks, .opaque = true, .planks = true, .breakSeconds = 1.2f, .drop = Block::k_BirchPlanks, .harvest = ToolType::k_Axe },
    { .id = Block::k_AcaciaPlanks, .opaque = true, .planks = true, .breakSeconds = 1.2f, .drop = Block::k_AcaciaPlanks, .harvest = ToolType::k_Axe },
    { .id = Block::k_Stone, .opaque = true, .breakSeconds = 1.5f, .drop = Block::k_Stone, .harvest = ToolType::k_Pickaxe },
    // Bedrock caps the world floor: no tool harvests it (k_None) and it drops
    // nothing, so it can be targeted but never broken or mined away.
    { .id = Block::k_Bedrock, .opaque = true, .harvest = ToolType::k_None },
} };

// Each row must line up with its enum value so a block indexes its own profile.
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

// A block's profile; every query below reads its fields.
inline constexpr const BlockInfo& BlockInfoOf(Block block)
{
    return k_BlockInfo[static_cast<size_t>(block)];
}

// A species' bark log (distinct end-grain top/bottom); "wood" wears bark on every
// face; "planks" are the crafted building cubes; "leaves" are biome-tinted canopy.
inline constexpr bool IsLog(Block block) { return BlockInfoOf(block).log; }
inline constexpr bool IsWood(Block block) { return BlockInfoOf(block).wood; }
inline constexpr bool IsPlanks(Block block) { return BlockInfoOf(block).planks; }
inline constexpr bool IsLeaves(Block block) { return BlockInfoOf(block).leaves; }

// Cross-shaped foliage (grass tufts, ferns, dead bushes): no collision, lets
// light through, never hides a neighbour's face.
inline constexpr bool IsPlant(Block block) { return BlockInfoOf(block).plant; }

// The pieces a tree is built from (trunk logs/wood plus canopy leaves); breaking
// one can leave the rest of the tree stranded in the air.
inline constexpr bool IsTreePart(Block block)
{
    const BlockInfo& info = BlockInfoOf(block);
    return info.log || info.wood || info.leaves;
}

// The blocks a generated tree is actually made of (wood and leaves): these
// crumble when a cut strands them off the ground. Logs are excluded on purpose -
// a player-placed log stays put like any other building block.
inline constexpr bool IsNaturalTreePart(Block block)
{
    const BlockInfo& info = BlockInfoOf(block);
    return info.wood || info.leaves;
}

// Whether the block hides the faces of blocks behind it (air, water and plants do
// not, so the seabed shows through water and dirt still draws under a grass tuft).
inline constexpr bool IsOpaque(Block block) { return BlockInfoOf(block).opaque; }

// Foliage with see-through texels (leaves, cactus): opaque for targeting and
// lighting, but its holes mean it never fully hides a neighbour's face, so the
// mesher keeps those faces (e.g. the dirt beneath a leaf, the sand under a cactus).
inline constexpr bool IsCutout(Block block) { return BlockInfoOf(block).cutout; }

// Blocks a raycast can target for breaking/placing: solids plus the pass-through
// plants (so a grass tuft can be clicked away though it neither collides nor
// occludes).
inline constexpr bool IsTargetable(Block block)
{
    const BlockInfo& info = BlockInfoOf(block);
    return info.opaque || info.plant;
}

// How long the proper tool takes to mine the block, in seconds, driving the
// gradual crack overlay. Plants pop instantly; trunk wood takes the longest.
inline constexpr float BreakSeconds(Block block) { return BlockInfoOf(block).breakSeconds; }

// What a broken block yields, or k_Air for nothing. Leaves and plants give way
// with no drop; a wood block yields its species' log (felling a trunk drops logs
// the way a tree does); everything else drops itself.
inline constexpr Block DropFor(Block block) { return BlockInfoOf(block).drop; }

// Which tools harvest the block (a mix is possible); k_None means nothing yet.
inline constexpr ToolType HarvestTools(Block block) { return BlockInfoOf(block).harvest; }

// Whether `tool` is one of the tools that harvests `block`.
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
    // Grayscale fringe drawn over the side and biome-tinted (grass). Unused
    // (left at the origin tile) by blocks that have no overlay.
    glm::vec2 sideOverlay;

private:
    inline static std::unordered_map<Block, BlockAtlas> s_BlockAtlases;
};

// The atlas tile a block shows when drawn as a flat icon (HUD slots and world
// drops). Most blocks read best as their side, but a log's bark is ambiguous
// flattened, so logs show their end-grain top tile instead.
inline glm::vec2 BlockIconTile(Block block)
{
    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);
    return IsLog(block) ? atlas.top : atlas.side;
}

} // namespace Krafter
