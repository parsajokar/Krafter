#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "glm/glm.hpp"

#include "FastNoiseLite.h"

#include "Krafter/World/Block.h"

namespace Krafter {

enum class BiomeType {
    k_Ocean,
    k_OakForest,
    k_BirchForest,
    k_Savannah,
    k_Desert,

    k_Count
};

struct Biome {
    BiomeType type;

    Block surface;
    Block subsurface;
    int32_t subsurfaceDepth;

    glm::vec3 grassColor;
    glm::vec3 leafColor;

    static const Biome& Get(BiomeType type);
    static const char* Name(BiomeType type);

    static BiomeType Select(float temperature, float humidity, float continentalness);
};

inline constexpr std::array<Biome, static_cast<size_t>(BiomeType::k_Count)> k_Biomes = { {
    { BiomeType::k_Ocean, Block::k_Sand, Block::k_Sand, 4,
        glm::vec3(0.569f, 0.741f, 0.349f), glm::vec3(0.471f, 0.671f, 0.302f) },
    { BiomeType::k_OakForest, Block::k_Grass, Block::k_Dirt, 4,
        glm::vec3(0.569f, 0.741f, 0.349f), glm::vec3(0.471f, 0.671f, 0.302f) },
    { BiomeType::k_BirchForest, Block::k_Grass, Block::k_Dirt, 4,
        glm::vec3(0.624f, 0.769f, 0.412f), glm::vec3(0.553f, 0.722f, 0.376f) },
    { BiomeType::k_Savannah, Block::k_Grass, Block::k_Dirt, 4,
        glm::vec3(0.741f, 0.722f, 0.357f), glm::vec3(0.643f, 0.639f, 0.318f) },
    { BiomeType::k_Desert, Block::k_Sand, Block::k_Sand, 4,
        glm::vec3(0.749f, 0.718f, 0.333f), glm::vec3(0.659f, 0.612f, 0.333f) },
} };

constexpr bool BiomeTableInOrder()
{
    for (size_t i = 0; i < k_Biomes.size(); ++i) {
        if (static_cast<size_t>(k_Biomes[i].type) != i) {
            return false;
        }
    }
    return true;
}
static_assert(BiomeTableInOrder(), "k_Biomes rows must follow the BiomeType enum order");

inline const Biome& Biome::Get(BiomeType type)
{
    return k_Biomes[static_cast<size_t>(type)];
}

class Climate {
public:
    explicit Climate(int32_t seed);

    float Temperature(float worldX, float worldZ) const;
    float Humidity(float worldX, float worldZ) const;
    float Continentalness(float worldX, float worldZ) const;

    BiomeType BiomeAt(float worldX, float worldZ) const;
    int32_t SurfaceHeight(float worldX, float worldZ) const;

private:
    FastNoiseLite m_TemperatureNoise;
    FastNoiseLite m_HumidityNoise;
    FastNoiseLite m_ContinentNoise;
    FastNoiseLite m_DetailNoise;

    FastNoiseLite m_ErosionNoise;
    FastNoiseLite m_WeirdnessNoise;
};

}
