#pragma once

#include <array>
#include <cstdint>

#include "glm/glm.hpp"

#include "Krafter/World/Chunk.h"

namespace Krafter {

inline constexpr std::array<glm::ivec3, 4> k_HorizontalNeighbors = {
    glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
};

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

inline glm::ivec2 ToChunkPosition(const glm::ivec3& worldPosition)
{
    return glm::ivec2(
        FloorDiv(worldPosition.x, Chunk::k_Width),
        FloorDiv(worldPosition.z, Chunk::k_Width));
}

inline glm::ivec3 ToLocalPosition(const glm::ivec3& worldPosition)
{
    return glm::ivec3(
        FloorMod(worldPosition.x, Chunk::k_Width),
        worldPosition.y,
        FloorMod(worldPosition.z, Chunk::k_Width));
}

}
