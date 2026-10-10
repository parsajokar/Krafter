#pragma once

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

class WorldAccess;

struct BlockBehavior {
    bool (*canSurvive)(const WorldAccess& world, const glm::ivec3& cell) = nullptr;

    void (*onUnsupported)(WorldAccess& world, const glm::ivec3& cell, Block self) = nullptr;

    void (*onNeighbourChanged)(WorldAccess& world, const glm::ivec3& cell, Block self) = nullptr;

    void (*onPlaced)(WorldAccess& world, const glm::ivec3& cell, Block self) = nullptr;

    void (*onRemoved)(WorldAccess& world, const glm::ivec3& cell, Block self, Block replacement) = nullptr;

    bool needsEmptyCell = false;
};

const BlockBehavior& BehaviorOf(Block block);

}
