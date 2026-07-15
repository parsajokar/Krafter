#include "Krafter/World/Chunk.h"

namespace Krafter {

Chunk::Chunk(const glm::ivec2& position)
    : m_Position(position)
    , m_Blocks(k_Width * k_Width * k_Height, Block::k_Air)
    , m_SkyLight(k_Width * k_Width * k_Height, 0)
    , m_BlockLight(k_Width * k_Width * k_Height, 0)
    , m_Fluid(k_Width * k_Width * k_Height, 0)
{
}

const Block& Chunk::GetBlock(const glm::ivec3& coords) const
{
    return m_Blocks[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x];
}

void Chunk::SetBlock(const glm::ivec3& coords, Block value)
{
    m_Blocks[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x] = value;
}

uint8_t Chunk::GetSkyLight(const glm::ivec3& coords) const
{
    return m_SkyLight[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x];
}

void Chunk::SetSkyLight(const glm::ivec3& coords, uint8_t value)
{
    m_SkyLight[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x] = value;
}

uint8_t Chunk::GetBlockLight(const glm::ivec3& coords) const
{
    return m_BlockLight[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x];
}

void Chunk::SetBlockLight(const glm::ivec3& coords, uint8_t value)
{
    m_BlockLight[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x] = value;
}

uint8_t Chunk::GetFluid(const glm::ivec3& coords) const
{
    return m_Fluid[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x];
}

void Chunk::SetFluid(const glm::ivec3& coords, uint8_t value)
{
    m_Fluid[(coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x] = value;
}

}
