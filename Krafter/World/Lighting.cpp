#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Coords.h"
#include "Krafter/World/Lighting.h"

namespace Krafter {

namespace {

constexpr int32_t k_Width = Chunk::k_Width;
constexpr int32_t k_Height = Chunk::k_Height;
constexpr int32_t k_Max = Chunk::k_MaxLight;

constexpr int32_t k_Lo = -k_Width;
constexpr int32_t k_Hi = 2 * k_Width - 1;
constexpr int32_t k_Span = 3 * k_Width;

constexpr int32_t Index(int32_t x, int32_t y, int32_t z)
{
    return (y * k_Span * k_Span) + ((z + k_Width) * k_Span) + (x + k_Width);
}

constexpr int32_t ColumnIndex(int32_t x, int32_t z)
{
    return ((z + k_Width) * k_Span) + (x + k_Width);
}

}

void ComputeSkyLight(Chunk& center, const std::array<const Chunk*, 9>& grid)
{
    auto isSolid = [&](int32_t x, int32_t y, int32_t z) -> bool {
        const int32_t cx = FloorDiv(x, k_Width);
        const int32_t cz = FloorDiv(z, k_Width);
        const Chunk* chunk = grid[(cz + 1) * 3 + (cx + 1)];
        if (!chunk) {
            return true;
        }
        const Block block = chunk->GetBlock(glm::ivec3(FloorMod(x, k_Width), y, FloorMod(z, k_Width)));
        return IsOpaque(block) && !IsCutout(block);
    };

    std::vector<uint8_t> solid(k_Span * k_Span * k_Height);
    for (int32_t x = k_Lo; x <= k_Hi; x++) {
        for (int32_t z = k_Lo; z <= k_Hi; z++) {
            for (int32_t y = 0; y < k_Height; y++) {
                solid[Index(x, y, z)] = isSolid(x, y, z) ? 1 : 0;
            }
        }
    }

    std::vector<int32_t> skyFloor(k_Span * k_Span, 0);
    std::vector<uint8_t> light(k_Span * k_Span * k_Height, 0);
    for (int32_t x = k_Lo; x <= k_Hi; x++) {
        for (int32_t z = k_Lo; z <= k_Hi; z++) {
            int32_t floor = 0;
            for (int32_t y = k_Height - 1; y >= 0; y--) {
                if (solid[Index(x, y, z)]) {
                    floor = y + 1;
                    break;
                }
            }
            skyFloor[ColumnIndex(x, z)] = floor;
            for (int32_t y = floor; y < k_Height; y++) {
                light[Index(x, y, z)] = k_Max;
            }
        }
    }

    constexpr int32_t hdx[] = { -1, 1, 0, 0 };
    constexpr int32_t hdz[] = { 0, 0, -1, 1 };

    std::vector<glm::ivec3> queue;
    for (int32_t x = k_Lo; x <= k_Hi; x++) {
        for (int32_t z = k_Lo; z <= k_Hi; z++) {
            const int32_t floor = skyFloor[ColumnIndex(x, z)];
            for (int32_t y = floor; y < k_Height; y++) {
                bool frontier = false;
                for (int32_t k = 0; k < 4; k++) {
                    const int32_t nx = x + hdx[k];
                    const int32_t nz = z + hdz[k];
                    if (nx < k_Lo || nx > k_Hi || nz < k_Lo || nz > k_Hi) {
                        continue;
                    }
                    if (y < skyFloor[ColumnIndex(nx, nz)]) {
                        frontier = true;
                        break;
                    }
                }
                if (frontier) {
                    queue.push_back(glm::ivec3(x, y, z));
                }
            }
        }
    }

    constexpr int32_t dx[] = { -1, 1, 0, 0, 0, 0 };
    constexpr int32_t dy[] = { 0, 0, -1, 1, 0, 0 };
    constexpr int32_t dz[] = { 0, 0, 0, 0, -1, 1 };

    for (size_t head = 0; head < queue.size(); head++) {
        const glm::ivec3 c = queue[head];
        const int32_t level = light[Index(c.x, c.y, c.z)];
        if (level <= 1) {
            continue;
        }

        for (int32_t k = 0; k < 6; k++) {
            const int32_t nx = c.x + dx[k];
            const int32_t ny = c.y + dy[k];
            const int32_t nz = c.z + dz[k];
            if (ny < 0 || ny >= k_Height) {
                continue;
            }
            if (nx < k_Lo || nx > k_Hi || nz < k_Lo || nz > k_Hi) {
                continue;
            }
            if (solid[Index(nx, ny, nz)] || light[Index(nx, ny, nz)] >= level - 1) {
                continue;
            }
            light[Index(nx, ny, nz)] = static_cast<uint8_t>(level - 1);
            queue.push_back(glm::ivec3(nx, ny, nz));
        }
    }

    for (int32_t x = 0; x < k_Width; x++) {
        for (int32_t y = 0; y < k_Height; y++) {
            for (int32_t z = 0; z < k_Width; z++) {
                center.SetSkyLight(glm::ivec3(x, y, z), light[Index(x, y, z)]);
            }
        }
    }
}

void ComputeBlockLight(Chunk& center, const std::array<const Chunk*, 9>& grid)
{
    auto blockAt = [&](int32_t x, int32_t y, int32_t z) -> Block {
        const int32_t cx = FloorDiv(x, k_Width);
        const int32_t cz = FloorDiv(z, k_Width);
        const Chunk* chunk = grid[(cz + 1) * 3 + (cx + 1)];
        if (!chunk) {
            return Block::k_Air;
        }
        return chunk->GetBlock(glm::ivec3(FloorMod(x, k_Width), y, FloorMod(z, k_Width)));
    };

    std::vector<uint8_t> solid(k_Span * k_Span * k_Height);
    std::vector<uint8_t> light(k_Span * k_Span * k_Height, 0);
    std::vector<glm::ivec3> queue;
    for (int32_t x = k_Lo; x <= k_Hi; x++) {
        for (int32_t z = k_Lo; z <= k_Hi; z++) {
            for (int32_t y = 0; y < k_Height; y++) {
                const Block block = blockAt(x, y, z);
                solid[Index(x, y, z)] = (IsOpaque(block) && !IsCutout(block)) ? 1 : 0;
                const uint8_t emission = LightEmission(block);
                if (emission > 0) {
                    light[Index(x, y, z)] = emission;
                    queue.push_back(glm::ivec3(x, y, z));
                }
            }
        }
    }

    constexpr int32_t dx[] = { -1, 1, 0, 0, 0, 0 };
    constexpr int32_t dy[] = { 0, 0, -1, 1, 0, 0 };
    constexpr int32_t dz[] = { 0, 0, 0, 0, -1, 1 };

    for (size_t head = 0; head < queue.size(); head++) {
        const glm::ivec3 c = queue[head];
        const int32_t level = light[Index(c.x, c.y, c.z)];
        if (level <= 1) {
            continue;
        }

        for (int32_t k = 0; k < 6; k++) {
            const int32_t nx = c.x + dx[k];
            const int32_t ny = c.y + dy[k];
            const int32_t nz = c.z + dz[k];
            if (ny < 0 || ny >= k_Height) {
                continue;
            }
            if (nx < k_Lo || nx > k_Hi || nz < k_Lo || nz > k_Hi) {
                continue;
            }
            if (solid[Index(nx, ny, nz)] || light[Index(nx, ny, nz)] >= level - 1) {
                continue;
            }
            light[Index(nx, ny, nz)] = static_cast<uint8_t>(level - 1);
            queue.push_back(glm::ivec3(nx, ny, nz));
        }
    }

    for (int32_t x = 0; x < k_Width; x++) {
        for (int32_t y = 0; y < k_Height; y++) {
            for (int32_t z = 0; z < k_Width; z++) {
                center.SetBlockLight(glm::ivec3(x, y, z), light[Index(x, y, z)]);
            }
        }
    }
}

}
