#include <cstring>
#include <iostream>

#include "FastNoiseLite.h"

#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

Chunk::Chunk(const glm::ivec2& position)
    : m_Position(position)
{
    m_Blocks = new Block[k_Width * k_Width * k_Height];
    memset(m_Blocks, 0, k_Width * k_Width * k_Height * sizeof(Block));

    m_SkyLight = new uint8_t[k_Width * k_Width * k_Height];
    memset(m_SkyLight, 0, k_Width * k_Width * k_Height * sizeof(uint8_t));

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.02f);

    FastNoiseLite biomeNoise;
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    biomeNoise.SetSeed(1337);
    biomeNoise.SetFrequency(0.0015f);

    for (int32_t x = 0; x < k_Width; x++) {
        for (int32_t z = 0; z < k_Width; z++) {
            const int32_t worldX = m_Position.x * k_Width + x;
            const int32_t worldZ = m_Position.y * k_Width + z;

            float noiseValue = noise.GetNoise((float)worldX, (float)worldZ); // Range = [-1, 1]

            float temperature = biomeNoise.GetNoise((float)worldX, (float)worldZ); // Range = [-1, 1]
            const Biome& biome = Biome::Get(Biome::Select(temperature));

            int32_t height = Biome::SampleHeight(temperature, noiseValue);

            for (int32_t y = 0; y < height; y++) {
                bool isSubsurface = (height - y) <= biome.subsurfaceDepth;
                SetBlock(glm::ivec3(x, y, z), isSubsurface ? biome.subsurface : Block::k_Dirt);
            }

            SetBlock(glm::ivec3(x, height, z), biome.surface);
        }
    }
}

Chunk::Chunk(const Chunk& other)
    : m_Position(other.m_Position)
{
    std::cout << "[CHUNK] Copying chunk at position (" << other.m_Position.x << ", " << other.m_Position.y << ")" << std::endl;

    m_Blocks = new Block[k_Width * k_Width * k_Height];
    m_SkyLight = new uint8_t[k_Width * k_Width * k_Height];
    for (uint32_t i = 0; i < k_Width * k_Width * k_Height; i++) {
        m_Blocks[i] = other.m_Blocks[i];
        m_SkyLight[i] = other.m_SkyLight[i];
    }
}

Chunk::Chunk(Chunk&& other)
    : m_Position(other.m_Position)
    , m_Blocks(other.m_Blocks)
    , m_SkyLight(other.m_SkyLight)
{
    other.m_Blocks = nullptr;
    other.m_SkyLight = nullptr;
}

Chunk::~Chunk()
{
    delete[] m_Blocks;
    delete[] m_SkyLight;
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

} // namespace Krafter
