#pragma once

#include <array>

#include "Krafter/World/Chunk.h"

namespace Krafter {

void ComputeSkyLight(Chunk& center, const std::array<const Chunk*, 9>& grid);

void ComputeBlockLight(Chunk& center, const std::array<const Chunk*, 9>& grid);

}
