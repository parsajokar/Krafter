#pragma once

#include <array>
#include <cstdint>

#include "glm/glm.hpp"

#include "Krafter/World/Chunk.h"

namespace Krafter {

// A cell's four horizontal neighbours (+x, -x, +z, -z). Shared by the checks that
// look at every side of a block, such as cactus placement and toppling.
inline constexpr std::array<glm::ivec3, 4> k_HorizontalNeighbors = {
    glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
};

// Floored division and modulo: they round toward negative infinity instead of
// truncating toward zero like C++'s built-in / and %. That keeps world-to-chunk
// mapping continuous across the origin, so the chunk left of x = 0 is -1 (not 0)
// and local coordinates stay in [0, k_Width).
constexpr int32_t FloorDiv(int32_t a, int32_t b)
{
    int32_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) {
        q--;
    }
    return q;
}

constexpr int32_t FloorMod(int32_t a, int32_t b)
{
    int32_t r = a % b;
    if (r != 0 && (r < 0) != (b < 0)) {
        r += b;
    }
    return r;
}

// The chunk column that contains a world position (ignoring height).
inline glm::ivec2 ToChunkPosition(const glm::ivec3& worldPosition)
{
    return glm::ivec2(
        FloorDiv(worldPosition.x, Chunk::k_Width),
        FloorDiv(worldPosition.z, Chunk::k_Width));
}

// A world position expressed relative to its own chunk: x and z wrapped into
// [0, k_Width), y left untouched.
inline glm::ivec3 ToLocalPosition(const glm::ivec3& worldPosition)
{
    return glm::ivec3(
        FloorMod(worldPosition.x, Chunk::k_Width),
        worldPosition.y,
        FloorMod(worldPosition.z, Chunk::k_Width));
}

} // namespace Krafter
