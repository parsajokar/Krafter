#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"

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

uint32_t Hash(int32_t x, int32_t z, uint32_t salt)
{
    uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(z) * 668265263u + salt * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

float Hash01(int32_t x, int32_t z, uint32_t salt)
{
    return Hash(x, z, salt) * (1.0f / 4294967296.0f);
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
// rolls no feature or its centre isn't on plains/desert land. Lakes form on
// plains, smaller and rarer oases in desert.
bool BuildLake(int32_t cellX, int32_t cellZ, Lake& lake)
{
    const int32_t centerX = cellX * k_LakeCellSize + static_cast<int32_t>(Hash01(cellX, cellZ, 1u) * k_LakeCellSize);
    const int32_t centerZ = cellZ * k_LakeCellSize + static_cast<int32_t>(Hash01(cellX, cellZ, 2u) * k_LakeCellSize);

    const BiomeType biome = Biome::At((float)centerX, (float)centerZ);

    const float roll = Hash01(cellX, cellZ, 3u);
    int32_t count;
    float baseRadius;
    if (biome == BiomeType::k_Plains) {
        if (roll >= 0.5f) {
            return false;
        }
        count = 4 + static_cast<int32_t>(Hash01(cellX, cellZ, 4u) * 4.0f); // 4..7
        baseRadius = 5.0f + Hash01(cellX, cellZ, 5u) * 3.0f;               // 5..8
    } else if (biome == BiomeType::k_Desert) {
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

} // namespace

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

    // Post-process: stamp inland lakes (plains) and oases (desert) as 3D blobs.
    const std::vector<Lake> lakes = GatherLakes(m_Position);
    for (const Lake& lake : lakes) {
        CarveLake(*this, m_Position, lake);
    }
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
