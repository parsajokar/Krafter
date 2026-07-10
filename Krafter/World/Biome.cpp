#include <iostream>
#include <stdexcept>

#include <cmath>

#include "FastNoiseLite.h"

#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

static FastNoiseLite s_TemperatureNoise;
static FastNoiseLite s_HumidityNoise;
static FastNoiseLite s_ContinentNoise;
static FastNoiseLite s_DetailNoise;

static FastNoiseLite s_ErosionNoise;
static FastNoiseLite s_WeirdnessNoise;

void Biome::Configure(int32_t seed)
{
    s_TemperatureNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_TemperatureNoise.SetSeed(1337 + seed);
    s_TemperatureNoise.SetFrequency(0.0009f);
    s_TemperatureNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    s_TemperatureNoise.SetFractalOctaves(3);
    s_TemperatureNoise.SetFractalGain(0.35f);

    s_HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_HumidityNoise.SetSeed(4242 + seed);
    s_HumidityNoise.SetFrequency(0.0011f);
    s_HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    s_HumidityNoise.SetFractalOctaves(3);
    s_HumidityNoise.SetFractalGain(0.35f);

    s_ContinentNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_ContinentNoise.SetSeed(2024 + seed);
    s_ContinentNoise.SetFrequency(0.0008f);

    s_DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_DetailNoise.SetSeed(1337 + seed);
    s_DetailNoise.SetFrequency(0.02f);

    s_ErosionNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_ErosionNoise.SetSeed(5150 + seed);
    s_ErosionNoise.SetFrequency(0.0012f);
    s_ErosionNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    s_ErosionNoise.SetFractalOctaves(3);
    s_ErosionNoise.SetFractalGain(0.4f);

    s_WeirdnessNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_WeirdnessNoise.SetSeed(9001 + seed);
    s_WeirdnessNoise.SetFrequency(0.0028f);
    s_WeirdnessNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    s_WeirdnessNoise.SetFractalOctaves(3);
    s_WeirdnessNoise.SetFractalGain(0.45f);

    LoadBiomes();
}

float Biome::Temperature(float worldX, float worldZ)
{
    return s_TemperatureNoise.GetNoise(worldX, worldZ);
}

float Biome::Humidity(float worldX, float worldZ)
{
    return s_HumidityNoise.GetNoise(worldX, worldZ);
}

float Biome::Continentalness(float worldX, float worldZ)
{
    return s_ContinentNoise.GetNoise(worldX, worldZ);
}

static float SmoothRamp(float value, float start, float end);

static float Spline(float t, const float* xs, const float* ys, int count)
{
    if (t <= xs[0]) {
        return ys[0];
    }
    for (int i = 1; i < count; i++) {
        if (t <= xs[i]) {
            const float u = (t - xs[i - 1]) / (xs[i] - xs[i - 1]);
            return ys[i - 1] + (ys[i] - ys[i - 1]) * u;
        }
    }
    return ys[count - 1];
}

static float ContinentalBase(float c)
{
    static const float xs[] = { -1.0f, -0.45f, -0.20f, -0.05f, 0.10f, 0.40f, 1.0f };
    static const float ys[] = { 18.0f, 40.0f, 58.0f, 63.0f, 70.0f, 82.0f, 92.0f };
    return Spline(c, xs, ys, 7);
}

static float ErosionRelief(float e)
{
    static const float xs[] = { -1.0f, -0.60f, -0.20f, 0.20f, 0.50f, 1.0f };
    static const float ys[] = { 60.0f, 45.0f, 24.0f, 9.0f, 4.0f, 2.0f };
    return Spline(e, xs, ys, 6);
}

static float PeaksValleys(float w)
{
    return -(std::fabs(std::fabs(w) * 3.0f - 2.0f) - 1.0f);
}

int32_t Biome::SurfaceHeight(float worldX, float worldZ)
{
    const float continentalness = Continentalness(worldX, worldZ);
    const float erosion = s_ErosionNoise.GetNoise(worldX, worldZ);
    const float peaksValleys = PeaksValleys(s_WeirdnessNoise.GetNoise(worldX, worldZ));
    const float detail = s_DetailNoise.GetNoise(worldX, worldZ);

    const float land = SmoothRamp(continentalness, -0.30f, 0.10f);

    const float height = ContinentalBase(continentalness)
        + ErosionRelief(erosion) * peaksValleys * land
        + detail * 4.0f;

    const int32_t rounded = static_cast<int32_t>(std::lround(height));
    if (rounded < 1) {
        return 1;
    }
    if (rounded > Chunk::k_Height - 2) {
        return Chunk::k_Height - 2;
    }
    return rounded;
}

BiomeType Biome::At(float worldX, float worldZ)
{
    return Select(Temperature(worldX, worldZ), Humidity(worldX, worldZ),
        Continentalness(worldX, worldZ));
}

const char* Biome::Name(BiomeType type)
{
    switch (type) {
    case BiomeType::k_Ocean:
        return "Ocean";
    case BiomeType::k_OakForest:
        return "Oak Forest";
    case BiomeType::k_BirchForest:
        return "Birch Forest";
    case BiomeType::k_Savannah:
        return "Savannah";
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
        .grassColor = glm::vec3(0.569f, 0.741f, 0.349f),
        .leafColor = glm::vec3(0.471f, 0.671f, 0.302f)
    };

    s_Biomes[BiomeType::k_OakForest] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .grassColor = glm::vec3(0.569f, 0.741f, 0.349f),
        .leafColor = glm::vec3(0.471f, 0.671f, 0.302f)
    };

    s_Biomes[BiomeType::k_BirchForest] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .grassColor = glm::vec3(0.624f, 0.769f, 0.412f),
        .leafColor = glm::vec3(0.553f, 0.722f, 0.376f)
    };

    s_Biomes[BiomeType::k_Savannah] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .grassColor = glm::vec3(0.741f, 0.722f, 0.357f),
        .leafColor = glm::vec3(0.643f, 0.639f, 0.318f)
    };

    s_Biomes[BiomeType::k_Desert] = {
        .surface = Block::k_Sand,
        .subsurface = Block::k_Sand,
        .subsurfaceDepth = 4,
        .grassColor = glm::vec3(0.749f, 0.718f, 0.333f),
        .leafColor = glm::vec3(0.659f, 0.612f, 0.333f)
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

static float SmoothRamp(float value, float start, float end)
{
    float t = (value - start) / (end - start);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

BiomeType Biome::Select(float temperature, float humidity, float continentalness)
{
    if (Landness(continentalness) < 0.5f) {
        return BiomeType::k_Ocean;
    }

    const bool hot = SmoothRamp(temperature, -0.2f, 0.2f) >= 0.5f;
    const bool dry = SmoothRamp(-humidity, -0.2f, 0.2f) >= 0.5f;

    if (hot) {
        return dry ? BiomeType::k_Desert : BiomeType::k_Savannah;
    }
    return dry ? BiomeType::k_BirchForest : BiomeType::k_OakForest;
}

float Biome::Landness(float continentalness)
{
    return SmoothRamp(continentalness, -0.3f, 0.0f);
}

}
