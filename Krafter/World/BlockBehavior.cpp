#include <array>
#include <unordered_set>
#include <vector>

#include "glm/gtx/hash.hpp"

#include "Krafter/World/BlockBehavior.h"
#include "Krafter/World/Chunk.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/Fluid.h"
#include "Krafter/World/WorldAccess.h"

namespace Krafter {

namespace {

constexpr glm::ivec3 k_Up(0, 1, 0);

constexpr glm::ivec3 k_Neighbors[6] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, 1, 0 }, { 0, -1, 0 }
};

bool PlantCanSurvive(const WorldAccess& world, const glm::ivec3& cell)
{
    return IsOpaque(world.GetBlock(cell - k_Up));
}

bool CactusCanSurvive(const WorldAccess& world, const glm::ivec3& cell)
{
    if (!IsOpaque(world.GetBlock(cell - k_Up))) {
        return false;
    }
    for (const glm::ivec3& side : k_HorizontalNeighbors) {
        if (IsOpaque(world.GetBlock(cell + side))) {
            return false;
        }
    }
    return true;
}

void CactusTopple(WorldAccess& world, const glm::ivec3& cell, Block)
{
    std::vector<glm::ivec3> column;
    for (glm::ivec3 above = cell; above.y < Chunk::k_Height; above += k_Up) {
        if (world.GetBlock(above) != Block::k_Cactus) {
            break;
        }
        column.push_back(above);
    }
    world.ScheduleFall(column, cell);
}

bool GemCanSurvive(const WorldAccess& world, const glm::ivec3& cell)
{
    for (const glm::ivec3& dir : k_Neighbors) {
        if (IsOpaque(world.GetBlock(cell + dir))) {
            return true;
        }
    }
    return false;
}

void BreakAndDrop(WorldAccess& world, const glm::ivec3& cell, Block self)
{
    if (world.SetBlock(cell, Block::k_Air)) {
        world.SpawnDrop(glm::vec3(cell) + 0.5f, DropItemFor(self));
    }
}

void ChopFloatingTree(WorldAccess& world, const glm::ivec3& brokenPosition, Block, Block replacement)
{
    if (IsNaturalTreePart(replacement)) {
        return;
    }

    auto forEachNeighbour = [](const glm::ivec3& cell, auto&& visit) {
        for (int32_t dy = -1; dy <= 1; dy++) {
            for (int32_t dz = -1; dz <= 1; dz++) {
                for (int32_t dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0 && dz == 0) {
                        continue;
                    }
                    visit(cell + glm::ivec3(dx, dy, dz));
                }
            }
        }
    };

    auto isStanding = [&world](const glm::ivec3& cell) {
        return IsNaturalTreePart(world.GetBlock(cell)) && !world.IsFalling(cell);
    };

    std::unordered_set<glm::ivec3> visited;
    std::vector<glm::ivec3> floating;

    std::vector<glm::ivec3> seeds;
    forEachNeighbour(brokenPosition, [&](const glm::ivec3& neighbour) { seeds.push_back(neighbour); });

    for (const glm::ivec3& seed : seeds) {
        if (visited.count(seed) || !isStanding(seed)) {
            continue;
        }

        std::vector<glm::ivec3> component;
        std::vector<glm::ivec3> stack { seed };
        visited.insert(seed);
        bool grounded = false;

        while (!stack.empty()) {
            const glm::ivec3 cell = stack.back();
            stack.pop_back();
            component.push_back(cell);

            for (int32_t dz = -1; dz <= 1 && !grounded; dz++) {
                for (int32_t dx = -1; dx <= 1 && !grounded; dx++) {
                    const Block under = world.GetBlock(cell + glm::ivec3(dx, -1, dz));
                    if (IsOpaque(under) && !IsNaturalTreePart(under)) {
                        grounded = true;
                    }
                }
            }

            forEachNeighbour(cell, [&](const glm::ivec3& next) {
                if (visited.count(next) || !isStanding(next)) {
                    return;
                }
                visited.insert(next);
                stack.push_back(next);
            });
        }

        if (grounded) {
            continue;
        }
        floating.insert(floating.end(), component.begin(), component.end());
    }

    world.ScheduleFall(floating, brokenPosition);
}

void FluidWake(WorldAccess& world, const glm::ivec3& cell, Block self)
{
    world.ScheduleFluid(cell, self);
}

std::array<BlockBehavior, static_cast<size_t>(Block::k_Count)> BuildBehaviors()
{
    std::array<BlockBehavior, static_cast<size_t>(Block::k_Count)> behaviors = {};
    for (size_t i = 0; i < behaviors.size(); i++) {
        const Block block = static_cast<Block>(i);
        BlockBehavior& behavior = behaviors[i];

        if (IsPlant(block)) {
            behavior.canSurvive = PlantCanSurvive;
            behavior.onUnsupported = BreakAndDrop;
            behavior.needsEmptyCell = true;
        } else if (block == Block::k_Cactus) {
            behavior.canSurvive = CactusCanSurvive;
            behavior.onUnsupported = CactusTopple;
        } else if (IsGem(block)) {
            behavior.canSurvive = GemCanSurvive;
            behavior.onUnsupported = BreakAndDrop;
        } else if (IsNaturalTreePart(block)) {
            behavior.onRemoved = ChopFloatingTree;
        } else if (IsFluid(block)) {
            behavior.onPlaced = FluidWake;
            behavior.onNeighbourChanged = FluidWake;
        }
    }
    return behaviors;
}

}

const BlockBehavior& BehaviorOf(Block block)
{
    static const std::array<BlockBehavior, static_cast<size_t>(Block::k_Count)> s_Behaviors = BuildBehaviors();
    return s_Behaviors[static_cast<size_t>(block)];
}

}
