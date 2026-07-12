#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

class World;
class WorldRenderer;

class DropSystem {
public:
    explicit DropSystem(const World& world);

    void Spawn(const glm::vec3& position, Block block);

    void Update(float deltaTime, const glm::vec3& cameraPosition);

    void Render(WorldRenderer& renderer, const glm::mat4& viewProjection) const;

    std::vector<Block> TakePickedUp()
    {
        return std::move(m_PickedUp);
    }

private:
    struct ItemDrop {
        glm::vec3 position;
        glm::vec3 velocity;
        Block block;
        float age;
        float phase;
    };

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
    static constexpr float k_DropBobSpeed = 3.0f;
    static constexpr float k_DropBobAmplitude = 0.1f;
    static constexpr float k_DropSize = 0.4f;

    const World& m_World;

    std::vector<ItemDrop> m_Drops;
    std::vector<Block> m_PickedUp;
    float m_Time = 0.0f;
    glm::vec3 m_LastCameraPosition = glm::vec3(0.0f);
};

}
