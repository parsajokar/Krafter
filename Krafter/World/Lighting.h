#pragma once

#include <array>
#include <functional>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Chunk.h"

namespace Krafter {

void ComputeSkyLight(std::vector<uint8_t>& out, const std::array<const Chunk*, 9>& grid);

void ComputeBlockLight(std::vector<uint8_t>& out, const std::array<const Chunk*, 9>& grid);

void UpdateLight(const std::array<Chunk*, 9>& area, const glm::ivec3& cell, Block previous,
    const std::function<void(const glm::ivec3&)>& onChanged);

}
