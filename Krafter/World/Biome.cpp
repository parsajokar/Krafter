#include <cmath>

#include "FastNoiseLite.h"

#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

namespace {

float SmoothRamp(float value, float start, float end)
{
    float t = (value - start) / (end - start);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

float Spline(float t, const float* xs, const float* ys, int count)
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

float ContinentalBase(float c)
{
    static const float xs[] = { -1.0f, -0.45f, -0.20f, -0.05f, 0.10f, 0.40f, 1.0f };
    static const float ys[] = { 18.0f, 40.0f, 58.0f, 63.0f, 70.0f, 82.0f, 92.0f };
    return Spline(c, xs, ys, 7);
}

float ErosionRelief(float e)
{
    static const float xs[] = { -1.0f, -0.60f, -0.20f, 0.20f, 0.50f, 1.0f };
    static const float ys[] = { 60.0f, 45.0f, 24.0f, 9.0f, 4.0f, 2.0f };
    return Spline(e, xs, ys, 6);
}

float PeaksValleys(float w)
{
    return -(std::fabs(std::fabs(w) * 3.0f - 2.0f) - 1.0f);
}

}

Climate::Climate(int32_t seed)
{
    m_TemperatureNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_TemperatureNoise.SetSeed(1337 + seed);
    m_TemperatureNoise.SetFrequency(0.0009f);
    m_TemperatureNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_TemperatureNoise.SetFractalOctaves(3);
    m_TemperatureNoise.SetFractalGain(0.35f);

    m_HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_HumidityNoise.SetSeed(4242 + seed);
    m_HumidityNoise.SetFrequency(0.0011f);
    m_HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_HumidityNoise.SetFractalOctaves(3);
    m_HumidityNoise.SetFractalGain(0.35f);

    m_ContinentNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_ContinentNoise.SetSeed(2024 + seed);
    m_ContinentNoise.SetFrequency(0.0008f);

    m_DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_DetailNoise.SetSeed(1337 + seed);
    m_DetailNoise.SetFrequency(0.02f);

    m_ErosionNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_ErosionNoise.SetSeed(5150 + seed);
    m_ErosionNoise.SetFrequency(0.0012f);
    m_ErosionNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_ErosionNoise.SetFractalOctaves(3);
    m_ErosionNoise.SetFractalGain(0.4f);

    m_WeirdnessNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_WeirdnessNoise.SetSeed(9001 + seed);
    m_WeirdnessNoise.SetFrequency(0.0028f);
    m_WeirdnessNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_WeirdnessNoise.SetFractalOctaves(3);
    m_WeirdnessNoise.SetFractalGain(0.45f);
}

float Climate::Temperature(float worldX, float worldZ) const
{
    return m_TemperatureNoise.GetNoise(worldX, worldZ);
}

float Climate::Humidity(float worldX, float worldZ) const
{
    return m_HumidityNoise.GetNoise(worldX, worldZ);
}

float Climate::Continentalness(float worldX, float worldZ) const
{
    return m_ContinentNoise.GetNoise(worldX, worldZ);
}

int32_t Climate::SurfaceHeight(float worldX, float worldZ) const
{
    const float continentalness = Continentalness(worldX, worldZ);
    const float erosion = m_ErosionNoise.GetNoise(worldX, worldZ);
    const float peaksValleys = PeaksValleys(m_WeirdnessNoise.GetNoise(worldX, worldZ));
    const float detail = m_DetailNoise.GetNoise(worldX, worldZ);

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

BiomeType Climate::BiomeAt(float worldX, float worldZ) const
{
    return Biome::Select(Temperature(worldX, worldZ), Humidity(worldX, worldZ),
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
    case BiomeType::k_Count:
        break;
    }
    return "Unknown";
}

BiomeType Biome::Select(float temperature, float humidity, float continentalness)
{
    if (SmoothRamp(continentalness, -0.3f, 0.0f) < 0.5f) {
        return BiomeType::k_Ocean;
    }

    const bool hot = SmoothRamp(temperature, -0.2f, 0.2f) >= 0.5f;
    const bool dry = SmoothRamp(-humidity, -0.2f, 0.2f) >= 0.5f;

    if (hot) {
        return dry ? BiomeType::k_Desert : BiomeType::k_Savannah;
    }
    return dry ? BiomeType::k_BirchForest : BiomeType::k_OakForest;
}

}
