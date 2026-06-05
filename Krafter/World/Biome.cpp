#include <iostream>
#include <stdexcept>

#include "Krafter/World/Biome.h"

namespace Krafter {

void Biome::LoadBiomes()
{
    s_Biomes[BiomeType::k_Plains] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .baseHeight = 64,
        .heightAmplitude = 32
    };

    s_Biomes[BiomeType::k_Desert] = {
        .surface = Block::k_Sand,
        .subsurface = Block::k_Sand,
        .subsurfaceDepth = 4,
        .baseHeight = 64,
        .heightAmplitude = 8
    };
}

const Biome& Biome::Get(BiomeType type)
{
    try {
        return s_Biomes.at(type);
    } catch (std::out_of_range e) {
        std::cerr << "Biome is not defined!" << std::endl;
        throw e;
    }
}

BiomeType Biome::Select(float temperature)
{
    if (temperature > 0.0f) {
        return BiomeType::k_Desert;
    }
    return BiomeType::k_Plains;
}

int32_t Biome::SampleHeight(float temperature, float noiseValue)
{
    const Biome& plains = Get(BiomeType::k_Plains);
    const Biome& desert = Get(BiomeType::k_Desert);

    constexpr float k_BlendStart = -0.2f;
    constexpr float k_BlendEnd = 0.2f;
    float t = (temperature - k_BlendStart) / (k_BlendEnd - k_BlendStart);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    float blend = t * t * (3.0f - 2.0f * t);

    float baseHeight = plains.baseHeight + (desert.baseHeight - plains.baseHeight) * blend;
    float amplitude = plains.heightAmplitude + (desert.heightAmplitude - plains.heightAmplitude) * blend;

    return (int32_t)(amplitude * noiseValue + baseHeight);
}

} // namespace Krafter
