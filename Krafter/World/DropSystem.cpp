#include <algorithm>
#include <cmath>

#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/DropSystem.h"
#include "Krafter/World/World.h"

namespace Krafter {

DropSystem::DropSystem(const World& world)
    : m_World(world)
{
}

void DropSystem::Spawn(const glm::vec3& position, Block block)
{
    if (block == Block::k_Air) {
        return;
    }

    const float angle = static_cast<float>(m_Drops.size()) * 2.39996323f;
    const glm::vec3 velocity(
        std::cos(angle) * k_DropPopOut, k_DropPopUp, std::sin(angle) * k_DropPopOut);
    m_Drops.push_back({ position, velocity, block, 0.0f, angle });
}

void DropSystem::Update(float deltaTime, const glm::vec3& cameraPosition)
{
    m_Time += deltaTime;
    m_LastCameraPosition = cameraPosition;

    for (size_t i = 0; i < m_Drops.size();) {
        ItemDrop& drop = m_Drops[i];
        drop.age += deltaTime;

        const glm::vec3 target = cameraPosition - glm::vec3(0.0f, k_PlayerEyeHeight, 0.0f);
        const glm::vec3 toTarget = target - drop.position;
        const float distSq = glm::dot(toTarget, toTarget);

        if (drop.age >= k_PickupDelay && distSq <= k_AttractRadius * k_AttractRadius) {
            if (distSq <= k_PickupRadius * k_PickupRadius) {
                m_PickedUp.push_back(drop.block);
                m_Drops[i] = m_Drops.back();
                m_Drops.pop_back();
                continue;
            }

            const float dist = std::sqrt(distSq);
            const float closeness = 1.0f - dist / k_AttractRadius;
            const float speed = glm::mix(k_AttractMinSpeed, k_AttractMaxSpeed, closeness);
            const float step = std::min(speed * deltaTime, dist);
            drop.position += (toTarget / dist) * step;
            drop.velocity = glm::vec3(0.0f);
            i++;
            continue;
        }

        drop.velocity.y -= k_DropGravity * deltaTime;
        drop.position += drop.velocity * deltaTime;

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

void DropSystem::Render(WorldRenderer& renderer, const glm::mat4& viewProjection) const
{
    constexpr glm::vec3 k_WorldUp(0.0f, 1.0f, 0.0f);
    for (const ItemDrop& drop : m_Drops) {
        glm::vec3 center = drop.position;
        center.y += std::sin((m_Time + drop.phase) * k_DropBobSpeed) * k_DropBobAmplitude;

        const glm::vec3 toEye = m_LastCameraPosition - center;
        if (glm::dot(toEye, toEye) < 1e-6f) {
            continue;
        }
        const glm::vec3 forward = glm::normalize(toEye);
        const glm::vec3 right = glm::normalize(glm::cross(k_WorldUp, forward));
        const glm::vec3 up = glm::cross(forward, right);

        renderer.RenderItemDrop(
            center, right * k_DropSize, up * k_DropSize, BlockIconTile(drop.block), viewProjection);
    }
}

}
