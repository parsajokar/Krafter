#include <iostream>
#include <stdexcept>

#include "FastNoiseLite.h"

#include "Krafter/World/Biome.h"

namespace Krafter {

// Configured once; FastNoiseLite::GetNoise is const, so concurrent reads from
// the chunk workers and the main thread are safe.
static const FastNoiseLite& TemperatureNoise()
{
    static const FastNoiseLite noise = [] {
        FastNoiseLite n;
        n.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        n.SetSeed(1337);
        n.SetFrequency(0.0015f);
        return n;
    }();
    return noise;
}

static const FastNoiseLite& ContinentNoise()
{
    static const FastNoiseLite noise = [] {
        FastNoiseLite n;
        n.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        n.SetSeed(2024);
        n.SetFrequency(0.0008f);
        return n;
    }();
    return noise;
}

float Biome::Temperature(float worldX, float worldZ)
{
    return TemperatureNoise().GetNoise(worldX, worldZ);
}

float Biome::Continentalness(float worldX, float worldZ)
{
    return ContinentNoise().GetNoise(worldX, worldZ);
}

// Fine-grained terrain detail driving the per-column height variation.
static const FastNoiseLite& DetailNoise()
{
    static const FastNoiseLite noise = [] {
        FastNoiseLite n;
        n.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        n.SetFrequency(0.02f);
        return n;
    }();
    return noise;
}

int32_t Biome::SurfaceHeight(float worldX, float worldZ)
{
    return SampleHeight(Temperature(worldX, worldZ), Continentalness(worldX, worldZ),
        DetailNoise().GetNoise(worldX, worldZ));
}

BiomeType Biome::At(float worldX, float worldZ)
{
    return Select(Temperature(worldX, worldZ), Continentalness(worldX, worldZ));
}

const char* Biome::Name(BiomeType type)
{
    switch (type) {
    case BiomeType::k_Ocean:
        return "Ocean";
    case BiomeType::k_Plains:
        return "Plains";
    case BiomeType::k_Desert:
        return "Desert";
    }
    return "Unknown";
}

void Biome::LoadBiomes()
{
    s_Biomes[BiomeType::k_Ocean] = {
        .surface = Block::k_Sand,
        .subsurface = Block::k_Sand,
        .subsurfaceDepth = 4,
        // Well below sea level, so ocean columns flood into deep water.
        .baseHeight = 40,
        .heightAmplitude = 5
    };

    // Floor (base - amplitude) sits above sea level so inland plains stay dry;
    // coastlines still dip into the sea via the continentalness blend.
    s_Biomes[BiomeType::k_Plains] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .baseHeight = 75,
        .heightAmplitude = 10
    };

    s_Biomes[BiomeType::k_Desert] = {
        .surface = Block::k_Sand,
        .subsurface = Block::k_Sand,
        .subsurfaceDepth = 4,
        .baseHeight = 72,
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

BiomeType Biome::Select(float temperature, float continentalness)
{
    if (Landness(continentalness) < 0.5f) {
        return BiomeType::k_Ocean;
    }
    if (temperature > 0.0f) {
        return BiomeType::k_Desert;
    }
    return BiomeType::k_Plains;
}

float Biome::DesertBlend(float temperature)
{
    constexpr float k_BlendStart = -0.2f;
    constexpr float k_BlendEnd = 0.2f;
    float t = (temperature - k_BlendStart) / (k_BlendEnd - k_BlendStart);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

float Biome::Landness(float continentalness)
{
    constexpr float k_OceanEnd = -0.3f;
    constexpr float k_LandStart = 0.0f;
    float t = (continentalness - k_OceanEnd) / (k_LandStart - k_OceanEnd);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

int32_t Biome::SampleHeight(float temperature, float continentalness, float noiseValue)
{
    const Biome& ocean = Get(BiomeType::k_Ocean);
    const Biome& plains = Get(BiomeType::k_Plains);
    const Biome& desert = Get(BiomeType::k_Desert);

    // Land terrain blends plains into desert by temperature.
    float landBlend = DesertBlend(temperature);
    float landBase = plains.baseHeight + (desert.baseHeight - plains.baseHeight) * landBlend;
    float landAmplitude = plains.heightAmplitude + (desert.heightAmplitude - plains.heightAmplitude) * landBlend;

    // Then ocean blends into that land as the coast rises out of the water.
    float land = Landness(continentalness);
    float baseHeight = ocean.baseHeight + (landBase - ocean.baseHeight) * land;
    float amplitude = ocean.heightAmplitude + (landAmplitude - ocean.heightAmplitude) * land;

    return (int32_t)(amplitude * noiseValue + baseHeight);
}

} // namespace Krafter
