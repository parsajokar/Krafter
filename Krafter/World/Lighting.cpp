#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

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

int32_t FloodHeight(const std::array<const Chunk*, 9>& grid)
{
    int32_t top = 0;
    for (const Chunk* chunk : grid) {
        if (chunk) {
            top = std::max(top, chunk->GetTopY());
        }
    }
    return std::min(k_Height, top + Chunk::k_SectionHeight);
}

}

void ComputeSkyLight(std::vector<uint8_t>& out, const std::array<const Chunk*, 9>& grid)
{
    auto isSolid = [&](int32_t x, int32_t y, int32_t z) -> bool {
        const int32_t cx = FloorDiv(x, k_Width);
        const int32_t cz = FloorDiv(z, k_Width);
        const Chunk* chunk = grid[(cz + 1) * 3 + (cx + 1)];
        if (!chunk) {
            return true;
        }
        return BlocksLight(chunk->GetBlock(glm::ivec3(FloorMod(x, k_Width), y, FloorMod(z, k_Width))));
    };

    const int32_t height = FloodHeight(grid);

    std::vector<uint8_t> solid(k_Span * k_Span * height);
    for (int32_t x = k_Lo; x <= k_Hi; x++) {
        for (int32_t z = k_Lo; z <= k_Hi; z++) {
            for (int32_t y = 0; y < height; y++) {
                solid[Index(x, y, z)] = isSolid(x, y, z) ? 1 : 0;
            }
        }
    }

    std::vector<int32_t> skyFloor(k_Span * k_Span, 0);
    std::vector<uint8_t> light(k_Span * k_Span * height, 0);
    for (int32_t x = k_Lo; x <= k_Hi; x++) {
        for (int32_t z = k_Lo; z <= k_Hi; z++) {
            int32_t floor = 0;
            for (int32_t y = height - 1; y >= 0; y--) {
                if (solid[Index(x, y, z)]) {
                    floor = y + 1;
                    break;
                }
            }
            skyFloor[ColumnIndex(x, z)] = floor;
            for (int32_t y = floor; y < height; y++) {
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
            for (int32_t y = floor; y < height; y++) {
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
            if (ny < 0 || ny >= height) {
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
                out[Chunk::Index(glm::ivec3(x, y, z))] = y < height ? light[Index(x, y, z)] : k_Max;
            }
        }
    }
}

void ComputeBlockLight(std::vector<uint8_t>& out, const std::array<const Chunk*, 9>& grid)
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

    const int32_t height = FloodHeight(grid);

    std::vector<uint8_t> solid(k_Span * k_Span * height);
    std::vector<uint8_t> light(k_Span * k_Span * height, 0);
    std::vector<glm::ivec3> queue;
    for (int32_t x = k_Lo; x <= k_Hi; x++) {
        for (int32_t z = k_Lo; z <= k_Hi; z++) {
            for (int32_t y = 0; y < height; y++) {
                const Block block = blockAt(x, y, z);
                solid[Index(x, y, z)] = BlocksLight(block) ? 1 : 0;
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
            if (ny < 0 || ny >= height) {
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
                out[Chunk::Index(glm::ivec3(x, y, z))] = y < height ? light[Index(x, y, z)] : 0;
            }
        }
    }
}

namespace {

class LightArea {
public:
    LightArea(const std::array<Chunk*, 9>& chunks)
        : m_Chunks(chunks)
        , m_Origin(chunks[4]->GetPosition() * Chunk::k_Width)
    {
    }

    Chunk* Resolve(const glm::ivec3& cell, glm::ivec3& local) const
    {
        if (cell.y < 0 || cell.y >= k_Height) {
            return nullptr;
        }
        const int32_t cx = FloorDiv(cell.x - m_Origin.x, k_Width);
        const int32_t cz = FloorDiv(cell.z - m_Origin.y, k_Width);
        if (cx < -1 || cx > 1 || cz < -1 || cz > 1) {
            return nullptr;
        }
        local = glm::ivec3(FloorMod(cell.x, k_Width), cell.y, FloorMod(cell.z, k_Width));
        return m_Chunks[(cz + 1) * 3 + (cx + 1)];
    }

    Block GetBlock(const glm::ivec3& cell) const
    {
        glm::ivec3 local;
        const Chunk* chunk = Resolve(cell, local);
        return chunk ? chunk->GetBlock(local) : Block::k_Air;
    }

    int32_t SkyFloor(int32_t x, int32_t z, const glm::ivec3& overrideCell, Block overrideBlock) const
    {
        glm::ivec3 local;
        const Chunk* chunk = Resolve(glm::ivec3(x, 0, z), local);
        if (!chunk) {
            return 0;
        }
        for (int32_t y = chunk->GetTopY() - 1; y >= 0; y--) {
            const glm::ivec3 cell(x, y, z);
            const Block block = cell == overrideCell ? overrideBlock : chunk->GetBlock(glm::ivec3(local.x, y, local.z));
            if (BlocksLight(block)) {
                return y + 1;
            }
        }
        return 0;
    }

private:
    std::array<Chunk*, 9> m_Chunks;
    glm::ivec2 m_Origin;
};

constexpr glm::ivec3 k_Directions[6] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
};

template <typename Get, typename Set, typename Source>
void Repropagate(const LightArea& area, const std::vector<glm::ivec3>& seeds, Get get, Set set, Source source,
    const std::function<void(const glm::ivec3&)>& onChanged)
{
    struct Removal {
        glm::ivec3 cell;
        uint8_t level;
    };
    std::vector<Removal> removals;
    std::vector<glm::ivec3> additions;

    auto write = [&](const glm::ivec3& cell, uint8_t level) {
        set(cell, level);
        onChanged(cell);
    };

    for (const glm::ivec3& seed : seeds) {
        const uint8_t old = get(seed);
        if (old > 0) {
            write(seed, 0);
            removals.push_back({ seed, old });
        }
    }

    for (size_t head = 0; head < removals.size(); head++) {
        const Removal removal = removals[head];
        for (const glm::ivec3& dir : k_Directions) {
            const glm::ivec3 next = removal.cell + dir;
            glm::ivec3 local;
            if (!area.Resolve(next, local)) {
                continue;
            }
            const uint8_t level = get(next);
            if (level == 0) {
                continue;
            }
            const uint8_t own = source(next);
            if (level < removal.level && own < level) {
                write(next, own);
                removals.push_back({ next, level });
                if (own > 0) {
                    additions.push_back(next);
                }
            } else {
                additions.push_back(next);
            }
        }
    }

    for (const glm::ivec3& seed : seeds) {
        const uint8_t own = source(seed);
        if (own > get(seed)) {
            write(seed, own);
        }
        additions.push_back(seed);
        for (const glm::ivec3& dir : k_Directions) {
            additions.push_back(seed + dir);
        }
    }

    for (size_t head = 0; head < additions.size(); head++) {
        const glm::ivec3 cell = additions[head];
        glm::ivec3 local;
        if (!area.Resolve(cell, local)) {
            continue;
        }
        const uint8_t level = get(cell);
        if (level <= 1) {
            continue;
        }
        for (const glm::ivec3& dir : k_Directions) {
            const glm::ivec3 next = cell + dir;
            if (!area.Resolve(next, local) || BlocksLight(area.GetBlock(next))) {
                continue;
            }
            if (get(next) < level - 1) {
                write(next, static_cast<uint8_t>(level - 1));
                additions.push_back(next);
            }
        }
    }
}

}

void UpdateLight(const std::array<Chunk*, 9>& area, const glm::ivec3& cell, Block previous,
    const std::function<void(const glm::ivec3&)>& onChanged)
{
    const LightArea light(area);
    const Block current = light.GetBlock(cell);
    const bool opacityChanged = BlocksLight(previous) != BlocksLight(current);

    auto getBlockLight = [&](const glm::ivec3& c) -> uint8_t {
        glm::ivec3 local;
        const Chunk* chunk = light.Resolve(c, local);
        return chunk ? chunk->GetBlockLight(local) : 0;
    };
    auto setBlockLight = [&](const glm::ivec3& c, uint8_t value) {
        glm::ivec3 local;
        if (Chunk* chunk = light.Resolve(c, local)) {
            chunk->SetBlockLight(local, value);
        }
    };
    auto blockSource = [&](const glm::ivec3& c) -> uint8_t {
        return LightEmission(light.GetBlock(c));
    };
    if (opacityChanged || LightEmission(previous) != LightEmission(current)) {
        Repropagate(light, { cell }, getBlockLight, setBlockLight, blockSource, onChanged);
    }

    if (!opacityChanged) {
        return;
    }

    const int32_t oldFloor = light.SkyFloor(cell.x, cell.z, cell, previous);
    const int32_t newFloor = light.SkyFloor(cell.x, cell.z, cell, current);
    std::vector<glm::ivec3> seeds { cell };
    for (int32_t y = std::min(oldFloor, newFloor); y < std::max(oldFloor, newFloor); y++) {
        if (y != cell.y) {
            seeds.push_back(glm::ivec3(cell.x, y, cell.z));
        }
    }

    std::unordered_map<glm::ivec2, int32_t> floors;
    auto skyFloor = [&](int32_t x, int32_t z) {
        auto [it, inserted] = floors.try_emplace(glm::ivec2(x, z), 0);
        if (inserted) {
            it->second = light.SkyFloor(x, z, glm::ivec3(INT32_MIN), Block::k_Air);
        }
        return it->second;
    };
    auto getSkyLight = [&](const glm::ivec3& c) -> uint8_t {
        glm::ivec3 local;
        const Chunk* chunk = light.Resolve(c, local);
        return chunk ? chunk->GetSkyLight(local) : 0;
    };
    auto setSkyLight = [&](const glm::ivec3& c, uint8_t value) {
        glm::ivec3 local;
        if (Chunk* chunk = light.Resolve(c, local)) {
            chunk->SetSkyLight(local, value);
        }
    };
    auto skySource = [&](const glm::ivec3& c) -> uint8_t {
        return !BlocksLight(light.GetBlock(c)) && c.y >= skyFloor(c.x, c.z) ? k_Max : 0;
    };
    Repropagate(light, seeds, getSkyLight, setSkyLight, skySource, onChanged);
}

}
