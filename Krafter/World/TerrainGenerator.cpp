#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "FastNoiseLite.h"

#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/Fluid.h"
#include "Krafter/World/TerrainGenerator.h"

namespace Krafter {

namespace {

constexpr int32_t k_AquiferBase = 48;
constexpr int32_t k_AquiferSwing = 22;
constexpr float k_AquiferWetCutoff = 0.2f;
constexpr int32_t k_AquiferGrid = 24;
constexpr int32_t k_AquiferQuantize = 4;
constexpr float k_AquiferBarrier = 3.0f;
constexpr int32_t k_AquiferTop = k_AquiferBase + k_AquiferSwing + 6;

constexpr int32_t k_LavaLevel = 10;

constexpr float k_EntranceThreshold = 0.55f;

constexpr int32_t k_LakeCellSize = 96;
constexpr int32_t k_MaxLakeReach = 20;
constexpr int32_t k_LakeSink = 2;

constexpr int32_t k_TreeCellSize = 16;
constexpr float k_TreeChance = 0.4f;
constexpr float k_SavannahTreeChance = 0.06f;
constexpr int32_t k_MaxTreeReach = 7;
constexpr int32_t k_TreeJitterMargin = 2;

constexpr int32_t k_Dir8X[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
constexpr int32_t k_Dir8Z[] = { 0, 1, 1, 1, 0, -1, -1, -1 };

constexpr float k_GrassPlantChance = 0.2f;
constexpr float k_CactusChance = 0.01f;
constexpr float k_DeadBushChance = 0.02f;
constexpr int32_t k_MaxCactusHeight = 3;
constexpr float k_CactusFlowerChance = 0.25f;
// Fraction of grass-plant rolls that become a flower instead of grass/fern.
constexpr float k_FlowerFraction = 0.1f;

// "Lion scratch" red-sand claw marks scored across desert sand.
constexpr int32_t k_ScratchGrid = 72;
constexpr float k_ScratchChance = 0.4f;
constexpr float k_ScratchMinLen = 12.0f;
constexpr float k_ScratchMaxLen = 22.0f;
constexpr float k_ClawSpacing = 2.5f;
constexpr float k_ClawHalfWidth = 0.75f;
constexpr float k_ScratchCurve = 2.0f;
constexpr int32_t k_ScratchReach = 16;
constexpr int32_t k_ScratchDepth = 3;

// Lone gems that stud ordinary cave walls very sparsely.
constexpr float k_GemChance = 0.0006f;

// Rare underground geode pockets that are entirely lined with one gem type.
// TEMP TEST VALUES: grid shrunk + chance raised so geodes are easy to find.
// Revert to grid 320 / chance 0.05f once confirmed.
constexpr int32_t k_GemClusterGrid = 80;
constexpr float k_GemClusterChance = 0.6f;
constexpr int32_t k_GemClusterMinY = 22;
constexpr int32_t k_GemClusterMaxY = 46;
constexpr int32_t k_GemClusterMinR = 4;
constexpr int32_t k_GemClusterMaxR = 6;

float GridJitter(int32_t x, int32_t z, uint32_t salt, uint32_t seed)
{
    uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(z) * 2246822519u
        + salt * 3266489917u + seed * 2166136261u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return h * (1.0f / 4294967296.0f);
}

}

TerrainGenerator::TerrainGenerator(int32_t seed)
    : m_Seed(static_cast<uint32_t>(seed))
{
    Biome::Configure(seed);

    m_CaveNoiseA.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_CaveNoiseA.SetSeed(7001 + static_cast<int>(m_Seed));
    m_CaveNoiseA.SetFrequency(0.025f);

    m_CaveNoiseB.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_CaveNoiseB.SetSeed(7919 + static_cast<int>(m_Seed));
    m_CaveNoiseB.SetFrequency(0.025f);

    m_CavernNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_CavernNoise.SetSeed(3121 + static_cast<int>(m_Seed));
    m_CavernNoise.SetFrequency(0.012f);
    m_CavernNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_CavernNoise.SetFractalOctaves(3);

    m_AquiferLevelNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_AquiferLevelNoise.SetSeed(5501 + static_cast<int>(m_Seed));
    m_AquiferLevelNoise.SetFrequency(0.006f);

    m_AquiferMaskNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_AquiferMaskNoise.SetSeed(8681 + static_cast<int>(m_Seed));
    m_AquiferMaskNoise.SetFrequency(0.004f);

    m_EntranceNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_EntranceNoise.SetSeed(4517 + static_cast<int>(m_Seed));
    m_EntranceNoise.SetFrequency(0.02f);

    m_CoalNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_CoalNoise.SetSeed(2237 + static_cast<int>(m_Seed));
    m_CoalNoise.SetFrequency(0.11f);

    m_CopperNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_CopperNoise.SetSeed(6373 + static_cast<int>(m_Seed));
    m_CopperNoise.SetFrequency(0.11f);

    m_IronNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_IronNoise.SetSeed(9491 + static_cast<int>(m_Seed));
    m_IronNoise.SetFrequency(0.12f);
}

uint32_t TerrainGenerator::Hash(int32_t x, int32_t z, uint32_t salt) const
{
    uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(z) * 668265263u
        + salt * 2246822519u + m_Seed * 3266489917u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

float TerrainGenerator::Hash01(int32_t x, int32_t z, uint32_t salt) const
{
    return Hash(x, z, salt) * (1.0f / 4294967296.0f);
}

int32_t TerrainGenerator::AquiferCellLevel(int32_t gx, int32_t gz, float cx, float cz) const
{
    if (m_AquiferMaskNoise.GetNoise(cx, cz) < k_AquiferWetCutoff) {
        return std::numeric_limits<int32_t>::min();
    }
    const int32_t raw = k_AquiferBase
        + static_cast<int32_t>(m_AquiferLevelNoise.GetNoise(cx, cz) * k_AquiferSwing);
    return (raw / k_AquiferQuantize) * k_AquiferQuantize;
}

Block TerrainGenerator::AquiferBlock(int32_t worldX, int32_t y, int32_t worldZ,
    int32_t surface, int32_t surfaceMargin) const
{
    if (y > surface - surfaceMargin - 1) {
        return Block::k_Air;
    }

    const int32_t cgx = FloorDiv(worldX, k_AquiferGrid);
    const int32_t cgz = FloorDiv(worldZ, k_AquiferGrid);

    float best1 = std::numeric_limits<float>::max();
    float best2 = std::numeric_limits<float>::max();
    int32_t level1 = std::numeric_limits<int32_t>::min();
    int32_t level2 = std::numeric_limits<int32_t>::min();

    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            const int32_t gx = cgx + dx;
            const int32_t gz = cgz + dz;
            const float cx = (static_cast<float>(gx) + GridJitter(gx, gz, 1u, m_Seed))
                * static_cast<float>(k_AquiferGrid);
            const float cz = (static_cast<float>(gz) + GridJitter(gx, gz, 2u, m_Seed))
                * static_cast<float>(k_AquiferGrid);
            const float ex = cx - static_cast<float>(worldX);
            const float ez = cz - static_cast<float>(worldZ);
            const float d = ex * ex + ez * ez;
            const int32_t level = AquiferCellLevel(gx, gz, cx, cz);
            if (d < best1) {
                best2 = best1;
                level2 = level1;
                best1 = d;
                level1 = level;
            } else if (d < best2) {
                best2 = d;
                level2 = level;
            }
        }
    }

    if (level1 == std::numeric_limits<int32_t>::min() || y > level1) {
        return Block::k_Air;
    }
    if (level2 == std::numeric_limits<int32_t>::min() || y > level2) {
        if (std::sqrt(best2) - std::sqrt(best1) < k_AquiferBarrier) {
            return Block::k_Stone;
        }
    }
    return Block::k_Water;
}

bool TerrainGenerator::Lake::Contains(int32_t x, int32_t y, int32_t z) const
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

float TerrainGenerator::Lake::SurfaceField(int32_t x, int32_t z) const
{
    float best = std::numeric_limits<float>::max();
    for (const Ellipsoid& e : blobs) {
        const float dyn = (cy - e.ey) / e.ry;
        const float t = 1.0f - dyn * dyn;
        if (t <= 0.0f) {
            continue;
        }
        const float rx = e.rx * std::sqrt(t);
        const float rz = e.rz * std::sqrt(t);
        const float dx = (x - e.ex) / rx;
        const float dz = (z - e.ez) / rz;
        best = std::min(best, dx * dx + dz * dz);
    }
    return best;
}

bool TerrainGenerator::BuildLake(int32_t cellX, int32_t cellZ, Lake& lake) const
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
        count = 4 + static_cast<int32_t>(Hash01(cellX, cellZ, 4u) * 4.0f);
        baseRadius = 5.0f + Hash01(cellX, cellZ, 5u) * 3.0f;
    } else if (biome == BiomeType::k_Savannah || biome == BiomeType::k_Desert) {
        if (roll >= 0.3f) {
            return false;
        }
        count = 3 + static_cast<int32_t>(Hash01(cellX, cellZ, 4u) * 3.0f);
        baseRadius = 3.0f + Hash01(cellX, cellZ, 5u) * 2.0f;
    } else {
        return false;
    }

    lake.cy = Biome::SurfaceHeight((float)centerX, (float)centerZ) - k_LakeSink;

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

bool TerrainGenerator::LakeContained(const Lake& lake) const
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
                    if (ay > Biome::SurfaceHeight((float)ax, (float)az)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

std::vector<TerrainGenerator::Lake> TerrainGenerator::GatherLakes(const glm::ivec2& chunkPosition) const
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

void TerrainGenerator::CarveLake(Chunk& chunk, const glm::ivec2& chunkPosition, const Lake& lake) const
{
    constexpr float k_BankReach = 0.8f;
    constexpr float k_BankCutoffSq = (1.0f + k_BankReach) * (1.0f + k_BankReach);
    constexpr int32_t k_BowlDepth = 3;

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
                const float f = (d - 1.0f) / k_BankReach;
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

bool TerrainGenerator::ColumnInLake(int32_t worldX, int32_t worldZ) const
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

bool TerrainGenerator::BuildTree(int32_t cellX, int32_t cellZ, Tree& tree) const
{
    const int32_t span = k_TreeCellSize - 2 * k_TreeJitterMargin;
    const int32_t x = cellX * k_TreeCellSize + k_TreeJitterMargin + static_cast<int32_t>(Hash(cellX, cellZ, 201u) % span);
    const int32_t z = cellZ * k_TreeCellSize + k_TreeJitterMargin + static_cast<int32_t>(Hash(cellX, cellZ, 202u) % span);

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

    const int32_t ground = Biome::SurfaceHeight((float)x, (float)z);
    if (ground <= Chunk::k_SeaLevel + 1 || ColumnInLake(x, z)) {
        return false;
    }

    tree.x = x;
    tree.z = z;
    tree.baseY = ground;
    tree.trunkHeight = 9 + static_cast<int32_t>(Hash(x, z, 203u) % 4u);
    tree.seed = Hash(x, z, 204u);
    return true;
}

std::vector<TerrainGenerator::Tree> TerrainGenerator::GatherTrees(const glm::ivec2& chunkPosition) const
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

void TerrainGenerator::PlaceTreeBlock(Chunk& chunk, const glm::ivec2& chunkPosition, int32_t worldX, int32_t y, int32_t worldZ, Block block) const
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

void TerrainGenerator::StampRootFlare(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree) const
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

void TerrainGenerator::StampAcacia(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree) const
{
    auto stampShelf = [&](int32_t cx, int32_t cz, int32_t cy, int32_t radius, uint32_t salt) {
        const int32_t r2 = radius * radius;
        for (int32_t dx = -radius; dx <= radius; dx++) {
            for (int32_t dz = -radius; dz <= radius; dz++) {
                const int32_t d2 = dx * dx + dz * dz;
                if (d2 > r2) {
                    continue;
                }
                if (d2 >= (radius - 1) * (radius - 1)
                    && Hash01(cx + dx, cz + dz, tree.seed + salt) < 0.3f) {
                    continue;
                }
                PlaceTreeBlock(chunk, chunkPosition, cx + dx, cy, cz + dz, tree.leaves);
            }
        }
    };

    const int32_t trunkH = 4 + static_cast<int32_t>(tree.seed % 3u);
    for (int32_t h = 1; h <= trunkH; h++) {
        PlaceTreeBlock(chunk, chunkPosition, tree.x, tree.baseY + h, tree.z, tree.wood);
    }
    StampRootFlare(chunk, chunkPosition, tree);
    const int32_t topY = tree.baseY + trunkH;

    constexpr int32_t k_AcaciaBranches = 6;
    for (int32_t b = 0; b < k_AcaciaBranches; b++) {
        const uint32_t r = Hash(tree.x, tree.z, tree.seed + 40u + static_cast<uint32_t>(b));
        const int32_t dir = static_cast<int32_t>((r + static_cast<uint32_t>(b) * 3u) % 8u);
        const int32_t outLen = 3 + static_cast<int32_t>(r % 2u);
        const int32_t startY = topY - static_cast<int32_t>((r >> 4) % 2u);

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

    stampShelf(tree.x, tree.z, topY + 3, 3, 4u);
}

void TerrainGenerator::StampLeafBlob(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree,
    int32_t cx, int32_t cz, int32_t cy, uint32_t salt) const
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
                    continue;
                }
                if (d2 == r2 && r2 > 1
                    && Hash01(cx + dx, cz + dz, tree.seed + salt) < 0.4f) {
                    continue;
                }
                PlaceTreeBlock(chunk, chunkPosition, cx + dx, y, cz + dz, tree.leaves);
            }
        }
    }
}

void TerrainGenerator::StampTree(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree) const
{
    if (tree.wood == Block::k_AcaciaWood) {
        StampAcacia(chunk, chunkPosition, tree);
        return;
    }

    const int32_t topLog = tree.baseY + tree.trunkHeight;
    for (int32_t h = 1; h <= tree.trunkHeight; h++) {
        PlaceTreeBlock(chunk, chunkPosition, tree.x, tree.baseY + h, tree.z, tree.wood);
    }
    StampRootFlare(chunk, chunkPosition, tree);

    constexpr int32_t k_LimbCount = 12;
    for (int32_t b = 0; b < k_LimbCount; b++) {
        const uint32_t r = Hash(tree.x, tree.z, tree.seed + 60u + static_cast<uint32_t>(b));
        const int32_t dir = static_cast<int32_t>(r % 8u);
        const int32_t startY = topLog - static_cast<int32_t>((r >> 3) % 6u);
        const int32_t len = 2 + static_cast<int32_t>((r >> 6) % 2u);

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

    StampLeafBlob(chunk, chunkPosition, tree, tree.x, tree.z, topLog + 1, 7u);
}

bool TerrainGenerator::OnLionScratch(int32_t worldX, int32_t worldZ) const
{
    const int32_t loX = FloorDiv(worldX - k_ScratchReach, k_ScratchGrid);
    const int32_t hiX = FloorDiv(worldX + k_ScratchReach, k_ScratchGrid);
    const int32_t loZ = FloorDiv(worldZ - k_ScratchReach, k_ScratchGrid);
    const int32_t hiZ = FloorDiv(worldZ + k_ScratchReach, k_ScratchGrid);

    for (int32_t cz = loZ; cz <= hiZ; cz++) {
        for (int32_t cx = loX; cx <= hiX; cx++) {
            if (Hash01(cx, cz, 600u) >= k_ScratchChance) {
                continue;
            }

            const float centerX = cx * k_ScratchGrid + Hash01(cx, cz, 601u) * k_ScratchGrid;
            const float centerZ = cz * k_ScratchGrid + Hash01(cx, cz, 602u) * k_ScratchGrid;
            if (Biome::At(centerX, centerZ) != BiomeType::k_Desert) {
                continue;
            }

            const float angle = Hash01(cx, cz, 603u) * 6.2831853f;
            const float dirX = std::cos(angle);
            const float dirZ = std::sin(angle);
            const int32_t claws = 3 + static_cast<int32_t>(Hash(cx, cz, 604u) % 2u);
            const float halfLen = 0.5f
                * (k_ScratchMinLen + Hash01(cx, cz, 605u) * (k_ScratchMaxLen - k_ScratchMinLen));

            const float dx = static_cast<float>(worldX) - centerX;
            const float dz = static_cast<float>(worldZ) - centerZ;
            const float along = dx * dirX + dz * dirZ;
            if (std::abs(along) > halfLen) {
                continue;
            }
            const float perp = -dx * dirZ + dz * dirX;

            // Claws bow together like a real drag and fan out toward the end.
            const float t = along / halfLen;
            const float curve = k_ScratchCurve * (1.0f - t * t);
            const float fan = 1.0f + 0.35f * t;
            for (int32_t k = 0; k < claws; k++) {
                const float lane = (static_cast<float>(k) - (claws - 1) * 0.5f) * k_ClawSpacing * fan;
                if (std::abs(perp - lane - curve) < k_ClawHalfWidth) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool TerrainGenerator::ColumnRollsCactus(int32_t worldX, int32_t worldZ) const
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

void TerrainGenerator::ScatterPlants(Chunk& chunk, const glm::ivec2& chunkPosition) const
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
            if (chunk.GetBlock(glm::ivec3(x, py, z)) != Block::k_Air) {
                continue;
            }

            const Block surface = chunk.GetBlock(glm::ivec3(x, ground, z));
            const float roll = Hash01(worldX, worldZ, 300u);

            if (surface == Block::k_Grass) {
                if (roll >= k_GrassPlantChance) {
                    continue;
                }
                const float pick = Hash01(worldX, worldZ, 301u);
                Block plant;
                if (pick < k_FlowerFraction) {
                    const uint32_t f = Hash(worldX, worldZ, 304u) % 3u;
                    plant = f == 0 ? Block::k_Rose : f == 1 ? Block::k_Dandelion : Block::k_Allium;
                } else if (pick < k_FlowerFraction + 0.2f) {
                    plant = Block::k_Fern;
                } else {
                    plant = Block::k_ShortGrass;
                }
                chunk.SetBlock(glm::ivec3(x, py, z), plant);
            } else if (surface == Block::k_Sand && biome == BiomeType::k_Desert) {
                if (roll < k_CactusChance) {
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
                        int32_t topY = -1;
                        for (int32_t h = 0; h < height; h++) {
                            const int32_t cy = py + h;
                            if (cy >= Chunk::k_Height || chunk.GetBlock(glm::ivec3(x, cy, z)) != Block::k_Air) {
                                break;
                            }
                            chunk.SetBlock(glm::ivec3(x, cy, z), Block::k_Cactus);
                            topY = cy;
                        }
                        const int32_t fy = topY + 1;
                        if (topY >= 0 && fy < Chunk::k_Height
                            && chunk.GetBlock(glm::ivec3(x, fy, z)) == Block::k_Air
                            && Hash01(worldX, worldZ, 303u) < k_CactusFlowerChance) {
                            chunk.SetBlock(glm::ivec3(x, fy, z), Block::k_CactusFlower);
                        }
                    }
                } else if (roll < k_CactusChance + k_DeadBushChance) {
                    chunk.SetBlock(glm::ivec3(x, py, z), Block::k_DeadBush);
                }
            }
        }
    }
}

void TerrainGenerator::CarveCaves(Chunk& chunk, const glm::ivec2& chunkPosition) const
{
    constexpr float k_TunnelHalfWidth = 0.08f;
    constexpr float k_CavernThreshold = 0.60f;
    constexpr int32_t k_SurfaceMargin = 5;

    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t z = 0; z < Chunk::k_Width; z++) {
            const int32_t worldX = chunkPosition.x * Chunk::k_Width + x;
            const int32_t worldZ = chunkPosition.y * Chunk::k_Width + z;

            const int32_t surface = Biome::SurfaceHeight((float)worldX, (float)worldZ);

            const bool land = surface > Chunk::k_SeaLevel + 3;
            const bool entrance = land
                && m_EntranceNoise.GetNoise((float)worldX, (float)worldZ) > k_EntranceThreshold;
            int32_t ceiling;
            if (entrance) {
                ceiling = surface;
            } else if (land) {
                ceiling = surface - k_SurfaceMargin;
            } else {
                ceiling = std::min(surface, Chunk::k_SeaLevel) - k_SurfaceMargin;
            }
            for (int32_t y = 1; y <= ceiling; y++) {
                const Block here = chunk.GetBlock(glm::ivec3(x, y, z));
                if (here != Block::k_Stone && here != Block::k_Dirt && here != Block::k_Sand) {
                    continue;
                }

                const float fx = static_cast<float>(worldX);
                const float fy = static_cast<float>(y);
                const float fz = static_cast<float>(worldZ);

                const bool tunnel = std::abs(m_CaveNoiseA.GetNoise(fx, fy, fz)) < k_TunnelHalfWidth
                    && std::abs(m_CaveNoiseB.GetNoise(fx, fy, fz)) < k_TunnelHalfWidth;
                const bool cavern = m_CavernNoise.GetNoise(fx, fy, fz) > k_CavernThreshold;

                if (tunnel || cavern) {
                    Block fill;
                    if (y <= k_LavaLevel) {
                        fill = Block::k_Lava;
                    } else if (y <= k_AquiferTop) {
                        fill = AquiferBlock(worldX, y, worldZ, surface, k_SurfaceMargin);
                    } else {
                        fill = Block::k_Air;
                    }
                    chunk.SetBlock(glm::ivec3(x, y, z), fill);
                }
            }

            if (entrance && surface < Chunk::k_Height
                && chunk.GetBlock(glm::ivec3(x, surface - 1, z)) == Block::k_Air) {
                chunk.SetBlock(glm::ivec3(x, surface, z), Block::k_Air);
            }
        }
    }
}

Block TerrainGenerator::OreFor(int32_t worldX, int32_t y, int32_t worldZ) const
{
    if (y < 4) {
        return Block::k_Air;
    }

    const float fx = static_cast<float>(worldX);
    const float fy = static_cast<float>(y);
    const float fz = static_cast<float>(worldZ);

    // Deepest and rarest first so overlaps keep the more valuable ore.
    if (y <= 60 && m_IronNoise.GetNoise(fx, fy, fz) > 0.60f) {
        return Block::k_IronOre;
    }
    if (y <= 100 && m_CopperNoise.GetNoise(fx, fy, fz) > 0.58f) {
        return Block::k_CopperOre;
    }
    if (m_CoalNoise.GetNoise(fx, fy, fz) > 0.55f) {
        return Block::k_CoalOre;
    }
    return Block::k_Air;
}

void TerrainGenerator::ScatterOres(Chunk& chunk, const glm::ivec2& chunkPosition) const
{
    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t z = 0; z < Chunk::k_Width; z++) {
            const int32_t worldX = chunkPosition.x * Chunk::k_Width + x;
            const int32_t worldZ = chunkPosition.y * Chunk::k_Width + z;

            for (int32_t y = 1; y < Chunk::k_Height; y++) {
                if (chunk.GetBlock(glm::ivec3(x, y, z)) != Block::k_Stone) {
                    continue;
                }
                const Block ore = OreFor(worldX, y, worldZ);
                if (ore != Block::k_Air) {
                    chunk.SetBlock(glm::ivec3(x, y, z), ore);
                }
            }
        }
    }
}

bool TerrainGenerator::BuildGemCluster(int32_t cellX, int32_t cellZ, GemCluster& cluster) const
{
    if (Hash01(cellX, cellZ, 900u) >= k_GemClusterChance) {
        return false;
    }

    const int32_t cx = cellX * k_GemClusterGrid
        + static_cast<int32_t>(Hash01(cellX, cellZ, 901u) * k_GemClusterGrid);
    const int32_t cz = cellZ * k_GemClusterGrid
        + static_cast<int32_t>(Hash01(cellX, cellZ, 902u) * k_GemClusterGrid);
    const int32_t cy = k_GemClusterMinY
        + static_cast<int32_t>(Hash01(cellX, cellZ, 903u) * (k_GemClusterMaxY - k_GemClusterMinY + 1));

    cluster.center = glm::ivec3(cx, cy, cz);
    cluster.radius = k_GemClusterMinR
        + static_cast<int32_t>(Hash(cellX, cellZ, 904u) % (k_GemClusterMaxR - k_GemClusterMinR + 1));
    return true;
}

std::vector<TerrainGenerator::GemCluster> TerrainGenerator::GatherGemClusters(const glm::ivec2& chunkPosition) const
{
    std::vector<GemCluster> clusters;

    const int32_t minX = chunkPosition.x * Chunk::k_Width - k_GemClusterMaxR;
    const int32_t maxX = chunkPosition.x * Chunk::k_Width + Chunk::k_Width - 1 + k_GemClusterMaxR;
    const int32_t minZ = chunkPosition.y * Chunk::k_Width - k_GemClusterMaxR;
    const int32_t maxZ = chunkPosition.y * Chunk::k_Width + Chunk::k_Width - 1 + k_GemClusterMaxR;

    for (int32_t cz = FloorDiv(minZ, k_GemClusterGrid); cz <= FloorDiv(maxZ, k_GemClusterGrid); cz++) {
        for (int32_t cx = FloorDiv(minX, k_GemClusterGrid); cx <= FloorDiv(maxX, k_GemClusterGrid); cx++) {
            GemCluster cluster;
            if (BuildGemCluster(cx, cz, cluster)) {
                clusters.push_back(cluster);
            }
        }
    }

    return clusters;
}

void TerrainGenerator::CarveGemCluster(Chunk& chunk, const glm::ivec2& chunkPosition, const GemCluster& cluster) const
{
    // Hollow out the interior and wrap it in an ice shell that the gems line.
    const int32_t r = cluster.radius;
    const float hollowSq = (r - 1.0f) * (r - 1.0f);
    const float outerSq = static_cast<float>(r * r);

    for (int32_t dy = -r; dy <= r; dy++) {
        const int32_t y = cluster.center.y + dy;
        if (y < 1 || y >= Chunk::k_Height) {
            continue;
        }
        for (int32_t dz = -r; dz <= r; dz++) {
            for (int32_t dx = -r; dx <= r; dx++) {
                const float d2 = static_cast<float>(dx * dx + dy * dy + dz * dz);
                if (d2 > outerSq) {
                    continue;
                }
                const int32_t lx = cluster.center.x + dx - chunkPosition.x * Chunk::k_Width;
                const int32_t lz = cluster.center.z + dz - chunkPosition.y * Chunk::k_Width;
                if (lx < 0 || lx >= Chunk::k_Width || lz < 0 || lz >= Chunk::k_Width) {
                    continue;
                }
                const glm::ivec3 cell(lx, y, lz);
                const Block cur = chunk.GetBlock(cell);
                if (cur != Block::k_Stone && !IsOre(cur)) {
                    continue;
                }
                if (d2 <= hollowSq) {
                    chunk.SetBlock(cell, Block::k_Air);
                } else {
                    // Shell is a mix of solid hard ice and translucent ice.
                    const Block shell = Hash01(cluster.center.x + dx, cluster.center.z + dz,
                                            908u + static_cast<uint32_t>(cluster.center.y + dy))
                            < 0.5f
                        ? Block::k_HardIce
                        : Block::k_Ice;
                    chunk.SetBlock(cell, shell);
                }
            }
        }
    }
}

void TerrainGenerator::ScatterCaveGems(Chunk& chunk, const glm::ivec2& chunkPosition) const
{
    static constexpr glm::ivec3 neighbors[6] = {
        { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
    };

    // Rare geode pockets: carve them first so their walls exist for the gem pass.
    const std::vector<GemCluster> clusters = GatherGemClusters(chunkPosition);
    for (const GemCluster& cluster : clusters) {
        CarveGemCluster(chunk, chunkPosition, cluster);
    }

    auto insideCluster = [&](int32_t wx, int32_t y, int32_t wz) -> bool {
        for (const GemCluster& cluster : clusters) {
            const int32_t dx = wx - cluster.center.x;
            const int32_t dy = y - cluster.center.y;
            const int32_t dz = wz - cluster.center.z;
            const float reach = cluster.radius + 0.5f;
            if (static_cast<float>(dx * dx + dy * dy + dz * dz) <= reach * reach) {
                return true;
            }
        }
        return false;
    };

    auto randomGem = [&](int32_t wx, int32_t y, int32_t wz) -> Block {
        const uint32_t t = Hash(wx, wz, 950u + static_cast<uint32_t>(y)) % 4u;
        return t == 0 ? Block::k_Topaz
            : t == 1  ? Block::k_Emerald
            : t == 2  ? Block::k_Amethyst
                      : Block::k_Diamond;
    };

    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t z = 0; z < Chunk::k_Width; z++) {
            const int32_t worldX = chunkPosition.x * Chunk::k_Width + x;
            const int32_t worldZ = chunkPosition.y * Chunk::k_Width + z;

            const int32_t surface = Biome::SurfaceHeight((float)worldX, (float)worldZ);
            const int32_t top = std::min(surface - 3, Chunk::k_Height - 2);

            for (int32_t y = 5; y <= top; y++) {
                if (chunk.GetBlock(glm::ivec3(x, y, z)) != Block::k_Air) {
                    continue;
                }

                bool touchesStone = false;
                bool touchesSolid = false;
                for (const glm::ivec3& dir : neighbors) {
                    const glm::ivec3 n(x + dir.x, y + dir.y, z + dir.z);
                    if (n.x < 0 || n.x >= Chunk::k_Width || n.z < 0 || n.z >= Chunk::k_Width
                        || n.y < 0 || n.y >= Chunk::k_Height) {
                        continue;
                    }
                    const Block nb = chunk.GetBlock(n);
                    if (nb == Block::k_Stone) {
                        touchesStone = true;
                    }
                    if (IsOpaque(nb)) {
                        touchesSolid = true;
                    }
                }

                // Inside a geode: line every wall with a random gem per block.
                if (insideCluster(worldX, y, worldZ)) {
                    if (touchesSolid) {
                        chunk.SetBlock(glm::ivec3(x, y, z), randomGem(worldX, y, worldZ));
                    }
                    continue;
                }

                // Otherwise, very sparse lone gems on ordinary cave walls.
                if (!touchesStone) {
                    continue;
                }
                if (Hash01(worldX, worldZ, 500u + static_cast<uint32_t>(y)) >= k_GemChance) {
                    continue;
                }

                const float pick = Hash01(worldX, worldZ, 700u + static_cast<uint32_t>(y));
                Block gem;
                if (y <= 20) {
                    gem = pick < 0.5f ? Block::k_Diamond : Block::k_Amethyst;
                } else if (y <= 40) {
                    gem = pick < 0.5f ? Block::k_Amethyst : Block::k_Emerald;
                } else if (y <= 70) {
                    gem = pick < 0.5f ? Block::k_Emerald : Block::k_Topaz;
                } else {
                    gem = Block::k_Topaz;
                }
                chunk.SetBlock(glm::ivec3(x, y, z), gem);
            }
        }
    }
}

void TerrainGenerator::Generate(Chunk& chunk, const glm::ivec2& position) const
{
    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t z = 0; z < Chunk::k_Width; z++) {
            const int32_t worldX = position.x * Chunk::k_Width + x;
            const int32_t worldZ = position.y * Chunk::k_Width + z;

            const BiomeType type = Biome::At((float)worldX, (float)worldZ);
            const Biome& biome = Biome::Get(type);

            const int32_t height = Biome::SurfaceHeight((float)worldX, (float)worldZ);

            for (int32_t y = 0; y < height; y++) {
                Block block;
                if (y == 0) {
                    block = Block::k_Bedrock;
                } else if (y <= 2 && Hash01(worldX, worldZ, 400u + static_cast<uint32_t>(y)) < 0.5f - 0.2f * (y - 1)) {
                    block = Block::k_Bedrock;
                } else if ((height - y) <= biome.subsurfaceDepth) {
                    block = biome.subsurface;
                } else {
                    block = Block::k_Stone;
                }
                chunk.SetBlock(glm::ivec3(x, y, z), block);
            }

            const bool sandyShore = height <= Chunk::k_SeaLevel + 1;
            chunk.SetBlock(glm::ivec3(x, height, z), sandyShore ? Block::k_Sand : biome.surface);

            if (!sandyShore && type == BiomeType::k_Desert && OnLionScratch(worldX, worldZ)) {
                for (int32_t y = height; y >= 0 && y > height - k_ScratchDepth; y--) {
                    if (chunk.GetBlock(glm::ivec3(x, y, z)) == Block::k_Sand) {
                        chunk.SetBlock(glm::ivec3(x, y, z), Block::k_RedSand);
                    }
                }
            }

            for (int32_t y = height + 1; y <= Chunk::k_SeaLevel; y++) {
                chunk.SetBlock(glm::ivec3(x, y, z), Block::k_Water);
            }
        }
    }

    CarveCaves(chunk, position);

    ScatterOres(chunk, position);
    ScatterCaveGems(chunk, position);

    const std::vector<Lake> lakes = GatherLakes(position);
    for (const Lake& lake : lakes) {
        CarveLake(chunk, position, lake);
    }

    static constexpr glm::ivec3 neighbors[6] = {
        { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
    };
    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t z = 0; z < Chunk::k_Width; z++) {
            for (int32_t y = 1; y < Chunk::k_Height; y++) {
                const glm::ivec3 cell(x, y, z);
                const Block block = chunk.GetBlock(cell);
                if (!IsFluid(block)) {
                    continue;
                }
                if (block == Block::k_Lava) {
                    bool touchesWater = false;
                    for (const glm::ivec3& dir : neighbors) {
                        const glm::ivec3 n(x + dir.x, y + dir.y, z + dir.z);
                        if (n.x < 0 || n.x >= Chunk::k_Width || n.z < 0 || n.z >= Chunk::k_Width
                            || n.y < 0 || n.y >= Chunk::k_Height) {
                            continue;
                        }
                        if (chunk.GetBlock(n) == Block::k_Water) {
                            touchesWater = true;
                            break;
                        }
                    }
                    if (touchesWater) {
                        chunk.SetBlock(cell, Block::k_Stone);
                        continue;
                    }
                }
                chunk.SetFluid(cell, k_FluidSource);
            }
        }
    }

    const std::vector<Tree> trees = GatherTrees(position);
    for (const Tree& tree : trees) {
        StampTree(chunk, position, tree);
    }

    ScatterPlants(chunk, position);
}

}
