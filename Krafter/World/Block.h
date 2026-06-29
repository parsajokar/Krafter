#pragma once

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
    k_Cactus
};

// The "log" blocks: a species' bark on the sides with a distinct end-grain tile
// on the top and bottom. Solid, full-colour cubes (not biome-tinted).
inline bool IsLog(Block block)
{
    return block == Block::k_OakLog || block == Block::k_BirchLog
        || block == Block::k_AcaciaLog;
}

// The "wood" blocks: a species' bark on every face (the side tile on top and
// bottom too, no end grain). What the trees are actually built from.
inline bool IsWood(Block block)
{
    return block == Block::k_OakWood || block == Block::k_BirchWood
        || block == Block::k_AcaciaWood;
}

// The canopy blocks of every tree species. Their tiles are grayscale, so the
// mesher tints them with the biome's leaf colour.
inline bool IsLeaves(Block block)
{
    return block == Block::k_OakLeaves || block == Block::k_BirchLeaves
        || block == Block::k_AcaciaLeaves;
}

// The pieces a tree is built from: the trunk's wood and logs plus the canopy
// leaves. Breaking one of these can leave the rest of the tree hanging in the
// air, with nothing connecting it back down to the ground.
inline bool IsTreePart(Block block)
{
    return IsLog(block) || IsWood(block) || IsLeaves(block);
}

// The blocks a generated tree is actually built from: trunk wood and canopy
// leaves. These crumble away when a cut strands them off the ground. Logs are
// excluded on purpose: they only ever enter the world by the player placing
// them, so an unsupported log stays put like any other building block rather
// than auto-destructing the way a chopped tree's remains do.
inline bool IsNaturalTreePart(Block block)
{
    return IsWood(block) || IsLeaves(block);
}

// Cross-shaped foliage (grass tufts, ferns, dead bushes): drawn as two crossed
// billboards instead of a cube. It has no collision, lets light through, and
// never hides a neighbour's face.
inline bool IsPlant(Block block)
{
    return block == Block::k_ShortGrass || block == Block::k_Fern
        || block == Block::k_DeadBush;
}

// Air, water, and cross plants do not hide the faces of blocks behind them, so
// neighbouring solids still mesh their touching faces (you can see the seabed
// through water, and the dirt beneath a grass tuft still draws its top).
inline bool IsOpaque(Block block)
{
    return block != Block::k_Air && block != Block::k_Water && !IsPlant(block);
}

// Blocks a raycast can target for breaking/placing: solid blocks plus the
// pass-through plants (so a grass tuft can be clicked away even though it
// neither collides nor occludes).
inline bool IsTargetable(Block block)
{
    return IsOpaque(block) || IsPlant(block);
}

// Foliage with see-through texels (leaves, cactus). It still counts as opaque
// for targeting and lighting, but its holes mean it never fully hides a
// neighbour's face, so the mesher keeps those faces (e.g. the dirt beneath a
// leaf block, or the sand a cactus stands on) instead of culling them.
inline bool IsCutout(Block block)
{
    return IsLeaves(block) || block == Block::k_Cactus;
}

// How long the proper tool takes to mine this block, in seconds. Drives the
// gradual break: the crack overlay advances through its stages as this much
// time accumulates under a held swing. Plants pop instantly, leaves give way
// quickly, and the solid trunk wood takes the longest.
inline float BreakSeconds(Block block)
{
    if (IsPlant(block)) {
        return 0.0f;
    }
    if (IsLeaves(block)) {
        return 0.3f;
    }
    if (block == Block::k_Cactus) {
        return 0.5f;
    }
    if (IsLog(block) || IsWood(block)) {
        return 1.2f;
    }
    return 0.75f;
}

// What a broken block yields as a collectible drop, or k_Air for nothing. Leaves
// and cross plants just give way, leaving nothing to pick up. A tree trunk is
// built from wood (bark on every face), but felling it yields logs (with end
// grain) the way a tree does in Minecraft, so each wood maps to its species' log.
// Everything else, logs and cactus included, drops itself.
inline Block DropFor(Block block)
{
    if (IsLeaves(block) || IsPlant(block)) {
        return Block::k_Air;
    }
    switch (block) {
    case Block::k_OakWood:
        return Block::k_OakLog;
    case Block::k_BirchWood:
        return Block::k_BirchLog;
    case Block::k_AcaciaWood:
        return Block::k_AcaciaLog;
    default:
        return block;
    }
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
