#pragma once

#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"
#include "Krafter/World/Item.h"

namespace Krafter {

class WorldAccess {
public:
    WorldAccess() = default;
    virtual ~WorldAccess() = default;

    WorldAccess(const WorldAccess&) = delete;
    WorldAccess& operator=(const WorldAccess&) = delete;

    virtual Block GetBlock(const glm::ivec3& cell) const = 0;
    virtual uint8_t GetFluid(const glm::ivec3& cell) const = 0;

    virtual bool SetBlock(const glm::ivec3& cell, Block block) = 0;

    virtual bool SetFluid(const glm::ivec3& cell, Block fluid, uint8_t level) = 0;

    virtual bool IsLoadedAround(const glm::ivec3& cell) const = 0;

    virtual bool IsLoadedOrKept(const glm::ivec3& cell) const = 0;

    virtual void SpawnDrop(const glm::vec3& position, const Item& item) = 0;

    virtual void ScheduleFall(const std::vector<glm::ivec3>& cells, const glm::ivec3& origin) = 0;
    virtual bool IsFalling(const glm::ivec3& cell) const = 0;

    virtual void ScheduleFluid(const glm::ivec3& cell, Block fluid) = 0;
};

}
