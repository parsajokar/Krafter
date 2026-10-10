#include <algorithm>
#include <cmath>

#include "Krafter/World/DropSystem.h"
#include "Krafter/World/WorldAccess.h"

namespace Krafter {

DropSystem::DropSystem(const WorldAccess& world)
    : m_World(world)
{
}

void DropSystem::Spawn(const glm::vec3& position, const Item& item)
{
    if (item.IsEmpty()) {
        return;
    }

    const float angle = static_cast<float>(m_Drops.size()) * 2.39996323f;
    const glm::vec3 velocity(
        std::cos(angle) * k_DropPopOut, k_DropPopUp, std::sin(angle) * k_DropPopOut);
    m_Drops.push_back({ position, position, velocity, item, 0.0f, angle });
}

void DropSystem::Tick(float step, const glm::vec3& collectorPosition, ItemCollector& collector)
{
    m_Time += step;
    m_LastStep = step;

    for (size_t i = 0; i < m_Drops.size();) {
        ItemDrop& drop = m_Drops[i];
        drop.previousPosition = drop.position;
        drop.age += step;

        const glm::vec3 target = collectorPosition - glm::vec3(0.0f, k_PlayerEyeHeight, 0.0f);
        const glm::vec3 toTarget = target - drop.position;
        const float distSq = glm::dot(toTarget, toTarget);

        if (drop.age >= k_PickupDelay && distSq <= k_AttractRadius * k_AttractRadius
            && collector.HasRoomFor(drop.item)) {
            if (distSq <= k_PickupRadius * k_PickupRadius) {
                drop.item = collector.Collect(drop.item);
                if (drop.item.IsEmpty()) {
                    m_Drops[i] = m_Drops.back();
                    m_Drops.pop_back();
                    continue;
                }
                i++;
                continue;
            }

            const float dist = std::sqrt(distSq);
            const float closeness = 1.0f - dist / k_AttractRadius;
            const float speed = glm::mix(k_AttractMinSpeed, k_AttractMaxSpeed, closeness);
            const float travel = std::min(speed * step, dist);
            drop.position += (toTarget / dist) * travel;
            drop.velocity = glm::vec3(0.0f);
            i++;
            continue;
        }

        drop.velocity.y -= k_DropGravity * step;
        drop.position += drop.velocity * step;

        const float feetY = drop.position.y - k_DropHalfHeight;
        const glm::ivec3 ground = glm::ivec3(glm::floor(
            glm::vec3(drop.position.x, feetY - 0.05f, drop.position.z)));
        if (drop.velocity.y <= 0.0f && IsOpaque(m_World.GetBlock(ground))) {
            drop.position.y = static_cast<float>(ground.y) + 1.0f + k_DropHalfHeight;
            drop.velocity = glm::vec3(0.0f);
        }

        if (drop.age >= k_DropLifetime || drop.position.y < k_DropVoidY) {
            m_Drops[i] = m_Drops.back();
            m_Drops.pop_back();
            continue;
        }

        i++;
    }
}

}
