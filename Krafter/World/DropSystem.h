#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Item.h"

namespace Krafter {

class WorldAccess;

class ItemCollector {
public:
    ItemCollector() = default;
    virtual ~ItemCollector() = default;

    ItemCollector(const ItemCollector&) = delete;
    ItemCollector& operator=(const ItemCollector&) = delete;

    virtual bool HasRoomFor(const Item& item) = 0;

    virtual Item Collect(const Item& item) = 0;
};

class DropSystem {
public:
    struct ItemDrop {
        glm::vec3 position;
        glm::vec3 previousPosition;
        glm::vec3 velocity;
        Item item;
        float age;
        float phase;
    };

    explicit DropSystem(const WorldAccess& world);

    void Spawn(const glm::vec3& position, const Item& item);

    void Tick(float step, const glm::vec3& collectorPosition, ItemCollector& collector);

    const std::vector<ItemDrop>& GetDrops() const
    {
        return m_Drops;
    }

    float GetTime(float alpha) const
    {
        return m_Time - (1.0f - alpha) * m_LastStep;
    }

private:
    static constexpr float k_DropGravity = 24.0f;
    static constexpr float k_DropPopUp = 3.0f;
    static constexpr float k_DropPopOut = 1.2f;
    static constexpr float k_DropHalfHeight = 0.125f;
    static constexpr float k_PickupDelay = 0.5f;
    static constexpr float k_AttractRadius = 4.0f;
    static constexpr float k_PickupRadius = 0.75f;
    static constexpr float k_AttractMinSpeed = 4.0f;
    static constexpr float k_AttractMaxSpeed = 16.0f;
    static constexpr float k_DropLifetime = 300.0f;
    static constexpr float k_PlayerEyeHeight = 1.62f;
    static constexpr float k_DropVoidY = -16.0f;

    const WorldAccess& m_World;

    std::vector<ItemDrop> m_Drops;
    float m_Time = 0.0f;
    float m_LastStep = 0.0f;
};

}
