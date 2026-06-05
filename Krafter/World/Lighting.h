#pragma once

#include <array>

#include "Krafter/World/Chunk.h"

namespace Krafter {

// Computes the sky light for `center` and writes it into the chunk's storage.
// `grid` is the 3x3 chunk neighbourhood, indexed by (dz + 1) * 3 + (dx + 1)
// (centre at index 4). The one-chunk apron exceeds the max light distance, so
// every chunk computes matching values at shared borders: no seams.
void ComputeSkyLight(Chunk& center, const std::array<const Chunk*, 9>& grid);

} // namespace Krafter
