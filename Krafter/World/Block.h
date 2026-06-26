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

} // namespace Krafter
