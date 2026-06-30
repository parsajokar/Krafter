#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"
#include "Krafter/World/Coords.h"

namespace Krafter {

namespace {

int32_t FloorDiv(int32_t a, int32_t b)
{
    int32_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) {
        q--;
    }
    return q;
}

// World seed mixed into every feature hash, so changing it reshuffles trees,
// plants, and lakes. Set once via Chunk::SetSeed before generation begins; 0
// reproduces the original fixed world.
static uint32_t s_WorldSeed = 0;

uint32_t Hash(int32_t x, int32_t z, uint32_t salt)
{
    uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(z) * 668265263u
        + salt * 2246822519u + s_WorldSeed * 3266489917u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

float Hash01(int32_t x, int32_t z, uint32_t salt)
{
    return Hash(x, z, salt) * (1.0f / 4294967296.0f);
}

// Lets Chunk::SetSeed (defined outside this anonymous namespace) reach the seed.
void StoreWorldSeed(uint32_t seed)
{
    s_WorldSeed = seed;
}

constexpr int32_t k_LakeCellSize = 96;
// Largest horizontal distance a blob (plus its sloped bank) can reach from its
// cell centre; used to know which neighbouring cells' lakes touch this chunk.
constexpr int32_t k_MaxLakeReach = 20;
// How far the water surface sits below the ground at the lake centre. Sinking
// the pool lets the surrounding ground wall it in, so ponds form readily rather
// than only in rare natural basins.
constexpr int32_t k_LakeSink = 2;

struct Ellipsoid {
    float ex, ey, ez; // centre, world coordinates
    float rx, ry, rz; // radii
};

// A lake/oasis: a lumpy union of ellipsoids. Cells at or below cy fill with
// water; cells above carve to air, opening the pool to the sky.
struct Lake {
    int32_t cy = 0; // water surface level
    int32_t minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
    std::vector<Ellipsoid> blobs;

    bool Contains(int32_t x, int32_t y, int32_t z) const
    {
        for (const Ellipsoid& e : blobs) {
            const float dx = (x - e.ex) / e.rx;
            const float dy = (y - e.ey) / e.ry;
            const float dz = (z - e.ez) / e.rz;
            if (dx * dx + dy * dy + dz * dz < 1.0f) {
                return true;
            }
        }
        return false;
    }

    // Normalised squared horizontal distance to the blob at the surface plane:
    // < 1 inside the pool footprint, growing outward for the sloped bank.
    float SurfaceField(int32_t x, int32_t z) const
    {
        float best = std::numeric_limits<float>::max();
        for (const Ellipsoid& e : blobs) {
            const float dyn = (cy - e.ey) / e.ry;
            const float t = 1.0f - dyn * dyn;
            if (t <= 0.0f) {
                continue; // ellipsoid doesn't reach the surface plane
            }
            const float rx = e.rx * std::sqrt(t);
            const float rz = e.rz * std::sqrt(t);
            const float dx = (x - e.ex) / rx;
            const float dz = (z - e.ez) / rz;
            best = std::min(best, dx * dx + dz * dz);
        }
        return best;
    }
};

// Deterministically builds the blob for a grid cell. Returns false if the cell
// rolls no feature or its centre isn't on land. Lakes form on the grassy forests,
// smaller and rarer oases in the savannah and desert.
bool BuildLake(int32_t cellX, int32_t cellZ, Lake& lake)
{
    const int32_t centerX = cellX * k_LakeCellSize + static_cast<int32_t>(Hash01(cellX, cellZ, 1u) * k_LakeCellSize);
    const int32_t centerZ = cellZ * k_LakeCellSize + static_cast<int32_t>(Hash01(cellX, cellZ, 2u) * k_LakeCellSize);

    const BiomeType biome = Biome::At((float)centerX, (float)centerZ);

    const float roll = Hash01(cellX, cellZ, 3u);
    int32_t count;
    float baseRadius;
    if (biome == BiomeType::k_OakForest || biome == BiomeType::k_BirchForest) {
        if (roll >= 0.5f) {
            return false;
        }
        count = 4 + static_cast<int32_t>(Hash01(cellX, cellZ, 4u) * 4.0f); // 4..7
        baseRadius = 5.0f + Hash01(cellX, cellZ, 5u) * 3.0f;               // 5..8
    } else if (biome == BiomeType::k_Savannah || biome == BiomeType::k_Desert) {
        // Arid biomes only hold the odd small waterhole / oasis.
        if (roll >= 0.3f) {
            return false;
        }
        count = 3 + static_cast<int32_t>(Hash01(cellX, cellZ, 4u) * 3.0f); // 3..5
        baseRadius = 3.0f + Hash01(cellX, cellZ, 5u) * 2.0f;               // 3..5
    } else {
        return false;
    }

    lake.cy = Biome::SurfaceHeight((float)centerX, (float)centerZ) - k_LakeSink;

    // Keep ponds inland and above the ocean; at/below sea level they would carve
    // into the sea and spill against it instead of being held by the land.
    if (lake.cy < Chunk::k_SeaLevel + 2) {
        return false;
    }

    lake.blobs.clear();
    lake.minX = lake.minY = lake.minZ = std::numeric_limits<int32_t>::max();
    lake.maxX = lake.maxY = lake.maxZ = std::numeric_limits<int32_t>::min();

    for (int32_t i = 0; i < count; i++) {
        const uint32_t salt = 100u + static_cast<uint32_t>(i) * 7u;
        Ellipsoid e;
        e.ex = centerX + (Hash01(cellX, cellZ, salt + 0u) - 0.5f) * baseRadius;
        e.ez = centerZ + (Hash01(cellX, cellZ, salt + 1u) - 0.5f) * baseRadius;
        e.ey = lake.cy + (Hash01(cellX, cellZ, salt + 2u) - 0.5f) * 3.0f;
        e.rx = 2.0f + Hash01(cellX, cellZ, salt + 3u) * (baseRadius - 2.0f);
        e.rz = 2.0f + Hash01(cellX, cellZ, salt + 4u) * (baseRadius - 2.0f);
        e.ry = 2.0f + Hash01(cellX, cellZ, salt + 5u) * 2.0f;
        lake.blobs.push_back(e);

        lake.minX = std::min(lake.minX, static_cast<int32_t>(std::floor(e.ex - e.rx)));
        lake.maxX = std::max(lake.maxX, static_cast<int32_t>(std::ceil(e.ex + e.rx)));
        lake.minY = std::min(lake.minY, static_cast<int32_t>(std::floor(e.ey - e.ry)));
        lake.maxY = std::max(lake.maxY, static_cast<int32_t>(std::ceil(e.ey + e.ry)));
        lake.minZ = std::min(lake.minZ, static_cast<int32_t>(std::floor(e.ez - e.rz)));
        lake.maxZ = std::max(lake.maxZ, static_cast<int32_t>(std::ceil(e.ez + e.rz)));
    }

    lake.minY = std::max(lake.minY, 0);
    lake.maxY = std::min(lake.maxY, Chunk::k_Height - 1);
    return true;
}

// A lake is valid only if every water cell is walled in on its sides and bottom
// by solid ground (or sea water); otherwise the pool would spill into open air.
// The top is left open as the water surface.
bool LakeContained(const Lake& lake)
{
    constexpr int32_t dx[] = { -1, 1, 0, 0, 0 };
    constexpr int32_t dy[] = { 0, 0, -1, 0, 0 };
    constexpr int32_t dz[] = { 0, 0, 0, -1, 1 };

    const int32_t topWater = std::min(lake.cy, lake.maxY);
    for (int32_t y = lake.minY; y <= topWater; y++) {
        for (int32_t z = lake.minZ; z <= lake.maxZ; z++) {
            for (int32_t x = lake.minX; x <= lake.maxX; x++) {
                if (!lake.Contains(x, y, z)) {
                    continue;
                }
                for (int32_t k = 0; k < 5; k++) {
                    const int32_t ax = x + dx[k];
                    const int32_t ay = y + dy[k];
                    const int32_t az = z + dz[k];
                    if (lake.Contains(ax, ay, az)) {
                        continue;
                    }
                    // Sides and bottom must be solid ground, or the pool leaks.
                    if (ay > Biome::SurfaceHeight((float)ax, (float)az)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

// Every contained lake whose blob can reach into this chunk.
std::vector<Lake> GatherLakes(const glm::ivec2& chunkPosition)
{
    std::vector<Lake> lakes;

    const int32_t minX = chunkPosition.x * Chunk::k_Width - k_MaxLakeReach;
    const int32_t maxX = chunkPosition.x * Chunk::k_Width + Chunk::k_Width - 1 + k_MaxLakeReach;
    const int32_t minZ = chunkPosition.y * Chunk::k_Width - k_MaxLakeReach;
    const int32_t maxZ = chunkPosition.y * Chunk::k_Width + Chunk::k_Width - 1 + k_MaxLakeReach;

    for (int32_t cz = FloorDiv(minZ, k_LakeCellSize); cz <= FloorDiv(maxZ, k_LakeCellSize); cz++) {
        for (int32_t cx = FloorDiv(minX, k_LakeCellSize); cx <= FloorDiv(maxX, k_LakeCellSize); cx++) {
            Lake lake;
            if (BuildLake(cx, cz, lake) && LakeContained(lake)) {
                lakes.push_back(std::move(lake));
            }
        }
    }

    return lakes;
}

// Carves the portion of a lake that falls inside this chunk. Pool columns get a
// shallow water-filled bowl open to the sky; the surrounding ring of bank
// columns is sloped down to the waterline so the edge isn't a vertical wall.
void CarveLake(Chunk& chunk, const glm::ivec2& chunkPosition, const Lake& lake)
{
    constexpr float k_BankReach = 0.8f; // extra normalised radius of sloped bank
    constexpr float k_BankCutoffSq = (1.0f + k_BankReach) * (1.0f + k_BankReach);
    constexpr int32_t k_BowlDepth = 3; // deepest water at the pool centre

    const int32_t surface = lake.cy;

    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t z = 0; z < Chunk::k_Width; z++) {
            const int32_t worldX = chunkPosition.x * Chunk::k_Width + x;
            const int32_t worldZ = chunkPosition.y * Chunk::k_Width + z;

            const float field = lake.SurfaceField(worldX, worldZ);
            if (field >= k_BankCutoffSq) {
                continue;
            }

            const int32_t ground = Biome::SurfaceHeight((float)worldX, (float)worldZ);
            const float d = std::sqrt(field);

            if (d < 1.0f) {
                // Pool: a shallow bowl (deeper toward the centre) filled flat.
                const int32_t bedDepth = static_cast<int32_t>(std::lround(k_BowlDepth * (1.0f - d)));
                const int32_t bottom = std::max(0, std::min(surface - bedDepth, ground + 1));
                for (int32_t y = bottom; y <= surface; y++) {
                    chunk.SetBlock(glm::ivec3(x, y, z), Block::k_Water);
                }
                if (bottom - 1 >= 0) {
                    const Block below = chunk.GetBlock(glm::ivec3(x, bottom - 1, z));
                    if (below != Block::k_Air && below != Block::k_Water) {
                        chunk.SetBlock(glm::ivec3(x, bottom - 1, z), Block::k_Sand);
                    }
                }
                for (int32_t y = surface + 1; y <= std::min(ground, Chunk::k_Height - 1); y++) {
                    chunk.SetBlock(glm::ivec3(x, y, z), Block::k_Air);
                }
            } else if (ground > surface) {
                // Bank: slope the ground from the waterline up to the surrounding
                // terrain so the pool edge reads as a shore, not a cliff.
                const float f = (d - 1.0f) / k_BankReach; // 0 at the waterline, 1 outside
                const int32_t targetTop = surface + static_cast<int32_t>(std::lround((ground - surface) * f));
                for (int32_t y = targetTop + 1; y <= std::min(ground, Chunk::k_Height - 1); y++) {
                    chunk.SetBlock(glm::ivec3(x, y, z), Block::k_Air);
                }
                if (targetTop >= 0 && targetTop < ground) {
                    const Block surfaceBlock = Biome::Get(Biome::At((float)worldX, (float)worldZ)).surface;
                    chunk.SetBlock(glm::ivec3(x, targetTop, z), surfaceBlock);
                }
            }
        }
    }
}

// Trees are scattered one per grid cell; the cell is large enough that the tall,
// broad-crowned trees of adjacent cells stand apart with room to breathe.
constexpr int32_t k_TreeCellSize = 16;
// Chance a forest cell rolls a tree, before the land/water checks reject it.
// Savannah is sparsely dotted with acacia, so its acacias are far rarer.
constexpr float k_TreeChance = 0.4f;
constexpr float k_SavannahTreeChance = 0.06f;
// Farthest a canopy leaf sits from the trunk column, so a chunk knows which
// neighbouring cells' trees can reach into it. The giant acacia umbrella is the
// widest: a branch leans four out before a radius-3 shelf, reaching seven out.
constexpr int32_t k_MaxTreeReach = 7;
// How far inside its cell a trunk stays. Smaller than the canopy reach so the
// trunks keep their spread (canopies may overlap a little between cells); it is
// the gather reach above, not this, that must cover the full canopy.
constexpr int32_t k_TreeJitterMargin = 2;

struct Tree {
    int32_t x = 0, baseY = 0, z = 0; // baseY is the grass column; the trunk sits above it
    int32_t trunkHeight = 0;         // number of trunk blocks
    uint32_t seed = 0;               // drives the deterministic corner pruning
    Block wood = Block::k_OakWood;   // species blocks, chosen by the biome below
    Block leaves = Block::k_OakLeaves;
};

// True if the column lies within a contained lake's pool or sloped bank, so a
// tree there would float over water or stand in the shore. Reuses the same
// deterministic lake field the carver does, so the two always agree.
bool ColumnInLake(int32_t worldX, int32_t worldZ)
{
    constexpr float k_BankCutoffSq = 1.8f * 1.8f;

    const int32_t loX = FloorDiv(worldX - k_MaxLakeReach, k_LakeCellSize);
    const int32_t hiX = FloorDiv(worldX + k_MaxLakeReach, k_LakeCellSize);
    const int32_t loZ = FloorDiv(worldZ - k_MaxLakeReach, k_LakeCellSize);
    const int32_t hiZ = FloorDiv(worldZ + k_MaxLakeReach, k_LakeCellSize);

    for (int32_t cz = loZ; cz <= hiZ; cz++) {
        for (int32_t cx = loX; cx <= hiX; cx++) {
            Lake lake;
            if (BuildLake(cx, cz, lake) && LakeContained(lake)
                && lake.SurfaceField(worldX, worldZ) < k_BankCutoffSq) {
                return true;
            }
        }
    }
    return false;
}

// Deterministically decides whether a tree grows in this cell and, if so, where,
// what species, and how tall. Returns false unless its trunk lands on dry grass
// of a tree-bearing biome (oak/birch forest or savannah).
bool BuildTree(int32_t cellX, int32_t cellZ, Tree& tree)
{
    // Jitter the trunk within the cell's interior, keeping a small margin off the
    // edges so neighbouring trunks stay a little apart.
    const int32_t span = k_TreeCellSize - 2 * k_TreeJitterMargin;
    const int32_t x = cellX * k_TreeCellSize + k_TreeJitterMargin + static_cast<int32_t>(Hash(cellX, cellZ, 201u) % span);
    const int32_t z = cellZ * k_TreeCellSize + k_TreeJitterMargin + static_cast<int32_t>(Hash(cellX, cellZ, 202u) % span);

    // Each forest grows its own species; savannah is the odd one out, sprinkling
    // acacia far more sparsely than the dense temperate forests.
    float chance;
    switch (Biome::At((float)x, (float)z)) {
    case BiomeType::k_OakForest:
        chance = k_TreeChance;
        tree.wood = Block::k_OakWood;
        tree.leaves = Block::k_OakLeaves;
        break;
    case BiomeType::k_BirchForest:
        chance = k_TreeChance;
        tree.wood = Block::k_BirchWood;
        tree.leaves = Block::k_BirchLeaves;
        break;
    case BiomeType::k_Savannah:
        chance = k_SavannahTreeChance;
        tree.wood = Block::k_AcaciaWood;
        tree.leaves = Block::k_AcaciaLeaves;
        break;
    default:
        return false;
    }

    if (Hash01(cellX, cellZ, 200u) >= chance) {
        return false;
    }

    // Above the shore sand band (so the trunk roots in grass) and clear of ponds.
    const int32_t ground = Biome::SurfaceHeight((float)x, (float)z);
    if (ground <= Chunk::k_SeaLevel + 1 || ColumnInLake(x, z)) {
        return false;
    }

    tree.x = x;
    tree.z = z;
    tree.baseY = ground;
    tree.trunkHeight = 9 + static_cast<int32_t>(Hash(x, z, 203u) % 4u); // 9..12 logs
    tree.seed = Hash(x, z, 204u);
    return true;
}

// Every tree whose canopy can reach into this chunk.
std::vector<Tree> GatherTrees(const glm::ivec2& chunkPosition)
{
    std::vector<Tree> trees;

    const int32_t minX = chunkPosition.x * Chunk::k_Width - k_MaxTreeReach;
    const int32_t maxX = chunkPosition.x * Chunk::k_Width + Chunk::k_Width - 1 + k_MaxTreeReach;
    const int32_t minZ = chunkPosition.y * Chunk::k_Width - k_MaxTreeReach;
    const int32_t maxZ = chunkPosition.y * Chunk::k_Width + Chunk::k_Width - 1 + k_MaxTreeReach;

    for (int32_t cz = FloorDiv(minZ, k_TreeCellSize); cz <= FloorDiv(maxZ, k_TreeCellSize); cz++) {
        for (int32_t cx = FloorDiv(minX, k_TreeCellSize); cx <= FloorDiv(maxX, k_TreeCellSize); cx++) {
            Tree tree;
            if (BuildTree(cx, cz, tree)) {
                trees.push_back(tree);
            }
        }
    }

    return trees;
}

// Writes one tree block into the chunk, clipping to its bounds (the rest is
// stamped by the neighbour that owns those columns). Leaves only fill air; a
// trunk block may also punch through a neighbouring tree's leaves so trunks stay solid.
void PlaceTreeBlock(Chunk& chunk, const glm::ivec2& chunkPosition, int32_t worldX, int32_t y, int32_t worldZ, Block block)
{
    if (y < 0 || y >= Chunk::k_Height) {
        return;
    }
    const int32_t lx = worldX - chunkPosition.x * Chunk::k_Width;
    const int32_t lz = worldZ - chunkPosition.y * Chunk::k_Width;
    if (lx < 0 || lx >= Chunk::k_Width || lz < 0 || lz >= Chunk::k_Width) {
        return;
    }

    const Block current = chunk.GetBlock(glm::ivec3(lx, y, lz));
    if (current == Block::k_Air || ((IsLog(block) || IsWood(block)) && IsLeaves(current))) {
        chunk.SetBlock(glm::ivec3(lx, y, lz), block);
    }
}

// The eight compass directions, used for the spread of acacia branches and oak
// limbs.
constexpr int32_t k_Dir8X[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
constexpr int32_t k_Dir8Z[] = { 0, 1, 1, 1, 0, -1, -1, -1 };

// Stamps the shared base flare: a plus of roots at baseY+1, each independently
// (a quarter of the time) fanning one block further out and stacking a block
// higher, so the trunk steps into the ground a little unevenly.
void StampRootFlare(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree)
{
    constexpr int32_t flareX[] = { 1, -1, 0, 0 };
    constexpr int32_t flareZ[] = { 0, 0, 1, -1 };
    for (int32_t f = 0; f < 4; f++) {
        PlaceTreeBlock(chunk, chunkPosition, tree.x + flareX[f], tree.baseY + 1, tree.z + flareZ[f], tree.wood);
        if (Hash01(tree.x, tree.z, tree.seed + 80u + static_cast<uint32_t>(f)) < 0.25f) {
            PlaceTreeBlock(chunk, chunkPosition, tree.x + 2 * flareX[f], tree.baseY + 1, tree.z + 2 * flareZ[f], tree.wood);
            PlaceTreeBlock(chunk, chunkPosition, tree.x + flareX[f], tree.baseY + 2, tree.z + flareZ[f], tree.wood);
        }
    }
}

// Stamps an acacia in the savannah silhouette: a short trunk that forks low into
// several branches fanning out and up, each topped by a wide, flat, one-block
// shelf of leaves. The overlapping shelves form acacia's broad, horizontal
// umbrella, tiered where branches of different length sit them at different
// heights.
void StampAcacia(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree)
{
    // A flat, one-block leaf shelf centred on a tip: a disc with a frayed rim.
    auto stampShelf = [&](int32_t cx, int32_t cz, int32_t cy, int32_t radius, uint32_t salt) {
        const int32_t r2 = radius * radius;
        for (int32_t dx = -radius; dx <= radius; dx++) {
            for (int32_t dz = -radius; dz <= radius; dz++) {
                const int32_t d2 = dx * dx + dz * dz;
                if (d2 > r2) {
                    continue; // round the square into a disc
                }
                if (d2 >= (radius - 1) * (radius - 1)
                    && Hash01(cx + dx, cz + dz, tree.seed + salt) < 0.3f) {
                    continue; // fray the rim
                }
                PlaceTreeBlock(chunk, chunkPosition, cx + dx, cy, cz + dz, tree.leaves);
            }
        }
    };

    // A tall-ish trunk on the shared base flare. Acacia still forks lower than
    // the oaks, but stands tall enough to carry a giant umbrella.
    const int32_t trunkH = 4 + static_cast<int32_t>(tree.seed % 3u); // 4..6
    for (int32_t h = 1; h <= trunkH; h++) {
        PlaceTreeBlock(chunk, chunkPosition, tree.x, tree.baseY + h, tree.z, tree.wood);
    }
    StampRootFlare(chunk, chunkPosition, tree);
    const int32_t topY = tree.baseY + trunkH;

    // Branches fork from the upper trunk and lean far out and up in spread
    // directions, each ending in a broad flat shelf. Varied fork height and
    // length set the shelves at a few heights, tiering the wide umbrella.
    constexpr int32_t k_AcaciaBranches = 6;
    for (int32_t b = 0; b < k_AcaciaBranches; b++) {
        const uint32_t r = Hash(tree.x, tree.z, tree.seed + 40u + static_cast<uint32_t>(b));
        const int32_t dir = static_cast<int32_t>((r + static_cast<uint32_t>(b) * 3u) % 8u);
        const int32_t outLen = 3 + static_cast<int32_t>(r % 2u);              // 3..4
        const int32_t startY = topY - static_cast<int32_t>((r >> 4) % 2u);    // topY-1..topY

        int32_t bx = tree.x;
        int32_t bz = tree.z;
        int32_t by = startY;
        for (int32_t step = 0; step < outLen; step++) {
            bx += k_Dir8X[dir];
            bz += k_Dir8Z[dir];
            by += 1;
            PlaceTreeBlock(chunk, chunkPosition, bx, by, bz, tree.wood);
        }
        stampShelf(bx, bz, by, 3, 5u + static_cast<uint32_t>(b));
    }

    // A flat central shelf over the trunk top keeps the umbrella's core covered.
    stampShelf(tree.x, tree.z, topY + 3, 3, 4u);
}

// A compact leaf blob: vertical radii 1/2/1 centred on (cx, cy, cz), each layer
// rounded to a disc with its rim frayed on a hashed coin flip so clusters read
// soft. Salted per call so the separate clusters of one tree fray differently.
// Hashing each leaf's own column keeps the choice identical from whichever chunk
// stamps it.
void StampLeafBlob(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree,
    int32_t cx, int32_t cz, int32_t cy, uint32_t salt)
{
    constexpr int32_t radii[] = { 1, 2, 1 };
    for (int32_t layer = 0; layer < 3; layer++) {
        const int32_t y = cy - 1 + layer;
        const int32_t radius = radii[layer];
        const int32_t r2 = radius * radius;
        for (int32_t dx = -radius; dx <= radius; dx++) {
            for (int32_t dz = -radius; dz <= radius; dz++) {
                const int32_t d2 = dx * dx + dz * dz;
                if (d2 > r2) {
                    continue; // round the square into a disc
                }
                if (d2 == r2 && r2 > 1
                    && Hash01(cx + dx, cz + dz, tree.seed + salt) < 0.4f) {
                    continue; // fray the rim
                }
                PlaceTreeBlock(chunk, chunkPosition, cx + dx, y, cz + dz, tree.leaves);
            }
        }
    }
}

// Stamps a big, natural tree: a tall one-block trunk that foots out into uneven
// roots, then throws a whorl of limbs out of its upper length at varied heights
// and directions, each climbing out and up to its own leaf cluster. The
// overlapping clusters build a broad, tiered crown with the branches showing
// through, rather than a solid ball. Acacia takes its own flat-topped form; the
// species' wood and leaf blocks come from the tree.
void StampTree(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree)
{
    if (tree.wood == Block::k_AcaciaWood) {
        StampAcacia(chunk, chunkPosition, tree);
        return;
    }

    // One-block trunk for the full height, on the shared pyramid base flare.
    const int32_t topLog = tree.baseY + tree.trunkHeight;
    for (int32_t h = 1; h <= tree.trunkHeight; h++) {
        PlaceTreeBlock(chunk, chunkPosition, tree.x, tree.baseY + h, tree.z, tree.wood);
    }
    StampRootFlare(chunk, chunkPosition, tree);

    // Limbs fork off the upper trunk at varied heights and head out in varied
    // directions, climbing a block per step. Lower, shorter limbs fill the wide
    // mid-canopy; higher ones raise the crown. Each ends in a leaf cluster.
    constexpr int32_t k_LimbCount = 12;
    for (int32_t b = 0; b < k_LimbCount; b++) {
        const uint32_t r = Hash(tree.x, tree.z, tree.seed + 60u + static_cast<uint32_t>(b));
        const int32_t dir = static_cast<int32_t>(r % 8u);
        const int32_t startY = topLog - static_cast<int32_t>((r >> 3) % 6u); // topLog-5 .. topLog
        const int32_t len = 2 + static_cast<int32_t>((r >> 6) % 2u);         // 2..3

        int32_t bx = tree.x;
        int32_t bz = tree.z;
        int32_t by = startY;
        for (int32_t step = 0; step < len; step++) {
            bx += k_Dir8X[dir];
            bz += k_Dir8Z[dir];
            by += 1;
            PlaceTreeBlock(chunk, chunkPosition, bx, by, bz, tree.wood);
        }
        StampLeafBlob(chunk, chunkPosition, tree, bx, bz, by, 20u + static_cast<uint32_t>(b));
    }

    // A leafy cap over the trunk top closes the crown at its peak.
    StampLeafBlob(chunk, chunkPosition, tree, tree.x, tree.z, topLog + 1, 7u);
}

// Fraction of grassy columns that sprout grass/ferns, and the (much sparser)
// fractions of desert sand columns that grow a cactus or a dead bush.
constexpr float k_GrassPlantChance = 0.2f;
constexpr float k_CactusChance = 0.01f;
constexpr float k_DeadBushChance = 0.02f;
// Tallest a cactus column grows.
constexpr int32_t k_MaxCactusHeight = 3;

// The deterministic per-column cactus roll, before adjacency rules. A column and
// its neighbours both consult this (across chunk borders, since it reads only
// noise) so they agree on where cacti want to grow and can keep clear of one
// another.
bool ColumnRollsCactus(int32_t worldX, int32_t worldZ)
{
    if (Biome::At((float)worldX, (float)worldZ) != BiomeType::k_Desert) {
        return false;
    }
    const int32_t ground = Biome::SurfaceHeight((float)worldX, (float)worldZ);
    if (ground <= Chunk::k_SeaLevel + 1 || ColumnInLake(worldX, worldZ)) {
        return false;
    }
    return Hash01(worldX, worldZ, 300u) < k_CactusChance;
}

// Scatters cross-shaped plants over the chunk's own surface: grass tufts and
// ferns on grass, dead bushes on desert sand. Each plant lives in a
// single column, so unlike trees it never spills across a border and can be
// stamped purely from this chunk's own blocks.
void ScatterPlants(Chunk& chunk, const glm::ivec2& chunkPosition)
{
    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t z = 0; z < Chunk::k_Width; z++) {
            const int32_t worldX = chunkPosition.x * Chunk::k_Width + x;
            const int32_t worldZ = chunkPosition.y * Chunk::k_Width + z;

            const BiomeType biome = Biome::At((float)worldX, (float)worldZ);
            if (biome == BiomeType::k_Ocean) {
                continue;
            }
            const int32_t ground = Biome::SurfaceHeight((float)worldX, (float)worldZ);
            if (ground <= Chunk::k_SeaLevel + 1 || ColumnInLake(worldX, worldZ)) {
                continue;
            }

            const int32_t py = ground + 1;
            if (py >= Chunk::k_Height) {
                continue;
            }
            // The cell above the surface must be clear (skip ponds and the columns
            // already taken by a tree trunk or canopy).
            if (chunk.GetBlock(glm::ivec3(x, py, z)) != Block::k_Air) {
                continue;
            }

            const Block surface = chunk.GetBlock(glm::ivec3(x, ground, z));
            const float roll = Hash01(worldX, worldZ, 300u);

            if (surface == Block::k_Grass) {
                // Mostly short grass with a sprinkling of ferns.
                if (roll >= k_GrassPlantChance) {
                    continue;
                }
                const Block plant = Hash01(worldX, worldZ, 301u) < 0.2f ? Block::k_Fern : Block::k_ShortGrass;
                chunk.SetBlock(glm::ivec3(x, py, z), plant);
            } else if (surface == Block::k_Sand && biome == BiomeType::k_Desert) {
                // Cactus columns and dead bushes dot the dunes.
                if (roll < k_CactusChance) {
                    // A cactus needs air on all four sides: no neighbouring cactus
                    // and no terrain rising to its base, so it never touches
                    // another block (matching the placement rule).
                    bool clear = true;
                    for (const glm::ivec3& side : k_HorizontalNeighbors) {
                        const int32_t ax = worldX + side.x;
                        const int32_t az = worldZ + side.z;
                        if (ColumnRollsCactus(ax, az) || Biome::SurfaceHeight((float)ax, (float)az) >= py) {
                            clear = false;
                            break;
                        }
                    }
                    if (clear) {
                        const int32_t height = 1 + static_cast<int32_t>(Hash(worldX, worldZ, 302u) % k_MaxCactusHeight);
                        for (int32_t h = 0; h < height; h++) {
                            const int32_t cy = py + h;
                            if (cy >= Chunk::k_Height || chunk.GetBlock(glm::ivec3(x, cy, z)) != Block::k_Air) {
                                break;
                            }
                            chunk.SetBlock(glm::ivec3(x, cy, z), Block::k_Cactus);
                        }
                    }
                } else if (roll < k_CactusChance + k_DeadBushChance) {
                    chunk.SetBlock(glm::ivec3(x, py, z), Block::k_DeadBush);
                }
            }
        }
    }
}

} // namespace

void Chunk::SetSeed(uint32_t seed)
{
    StoreWorldSeed(seed);
}

Chunk::Chunk(const glm::ivec2& position)
    : m_Position(position)
{
    m_Blocks = new Block[k_Width * k_Width * k_Height];
    memset(m_Blocks, 0, k_Width * k_Width * k_Height * sizeof(Block));

    m_SkyLight = new uint8_t[k_Width * k_Width * k_Height];
    memset(m_SkyLight, 0, k_Width * k_Width * k_Height * sizeof(uint8_t));

    // Base terrain: surface, sandy shore, and the global ocean fill.
    for (int32_t x = 0; x < k_Width; x++) {
        for (int32_t z = 0; z < k_Width; z++) {
            const int32_t worldX = m_Position.x * k_Width + x;
            const int32_t worldZ = m_Position.y * k_Width + z;

            const BiomeType type = Biome::At((float)worldX, (float)worldZ);
            const Biome& biome = Biome::Get(type);

            const int32_t height = Biome::SurfaceHeight((float)worldX, (float)worldZ);

            for (int32_t y = 0; y < height; y++) {
                bool isSubsurface = (height - y) <= biome.subsurfaceDepth;
                SetBlock(glm::ivec3(x, y, z), isSubsurface ? biome.subsurface : Block::k_Dirt);
            }

            // Seabed and the shore band just above the waterline read as sand.
            const bool sandyShore = height <= k_SeaLevel + 1;
            SetBlock(glm::ivec3(x, height, z), sandyShore ? Block::k_Sand : biome.surface);

            for (int32_t y = height + 1; y <= k_SeaLevel; y++) {
                SetBlock(glm::ivec3(x, y, z), Block::k_Water);
            }
        }
    }

    // Post-process: stamp inland lakes (forests) and oases (savannah/desert) as 3D blobs.
    const std::vector<Lake> lakes = GatherLakes(m_Position);
    for (const Lake& lake : lakes) {
        CarveLake(*this, m_Position, lake);
    }

    // Then scatter trees over the dry land, including the parts of trees
    // rooted in neighbouring chunks whose canopies spill across the border.
    const std::vector<Tree> trees = GatherTrees(m_Position);
    for (const Tree& tree : trees) {
        StampTree(*this, m_Position, tree);
    }

    // Finally, dust the grass with low foliage, after trees so it can
    // avoid the columns their trunks and canopies occupy.
    ScatterPlants(*this, m_Position);
}

Chunk::Chunk(const Chunk& other)
    : m_Position(other.m_Position)
{
    std::cout << "[CHUNK] Copying chunk at position (" << other.m_Position.x << ", " << other.m_Position.y << ")" << std::endl;

    m_Blocks = new Block[k_Width * k_Width * k_Height];
    m_SkyLight = new uint8_t[k_Width * k_Width * k_Height];
    for (uint32_t i = 0; i < k_Width * k_Width * k_Height; i++) {
        m_Blocks[i] = other.m_Blocks[i];
        m_SkyLight[i] = other.m_SkyLight[i];
    }
}

Chunk::Chunk(Chunk&& other)
    : m_Position(other.m_Position)
    , m_Blocks(other.m_Blocks)
    , m_SkyLight(other.m_SkyLight)
{
    other.m_Blocks = nullptr;
    other.m_SkyLight = nullptr;
}

Chunk::~Chunk()
{
    delete[] m_Blocks;
    delete[] m_SkyLight;
}

const Block& Chunk::GetBlock(const glm::ivec3& coords) const
{
    return m_Blocks[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x];
}

void Chunk::SetBlock(const glm::ivec3& coords, Block value)
{
    m_Blocks[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x] = value;
}

uint8_t Chunk::GetSkyLight(const glm::ivec3& coords) const
{
    return m_SkyLight[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x];
}

void Chunk::SetSkyLight(const glm::ivec3& coords, uint8_t value)
{
    m_SkyLight[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x] = value;
}

} // namespace Krafter
