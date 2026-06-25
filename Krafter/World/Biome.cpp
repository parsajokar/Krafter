#include <iostream>
#include <stdexcept>

#include "FastNoiseLite.h"

#include "Krafter/World/Biome.h"

namespace Krafter {

// Configured once; FastNoiseLite::GetNoise is const, so concurrent reads from
// the chunk workers and the main thread are safe.
//
// Climate fields are fractal (FBm) rather than a single smooth octave. A single
// octave has near-straight contour lines, so thresholding it for a biome border
// produces unnaturally straight edges; stacking octaves wrinkles the contour at
// every scale, the way Minecraft's multi-noise climate does.
static const FastNoiseLite& TemperatureNoise()
{
    static const FastNoiseLite noise = [] {
        FastNoiseLite n;
        n.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        n.SetSeed(1337);
        // Low base frequency keeps biomes large; the threshold is crossed on
        // this scale. The few low-gain octaves only ride on top to wrinkle the
        // border without spawning small islands of the other biome.
        n.SetFrequency(0.0009f);
        n.SetFractalType(FastNoiseLite::FractalType_FBm);
        n.SetFractalOctaves(3);
        n.SetFractalGain(0.35f);
        return n;
    }();
    return noise;
}

// Second, independent climate axis. Desert is the hot *and* dry corner of the
// (temperature, humidity) plane, so the border no longer follows the contour of
// a single field — it's where two unrelated noise fields jointly cross out of
// the desert box, which is far less regular than either field alone.
static const FastNoiseLite& HumidityNoise()
{
    static const FastNoiseLite noise = [] {
        FastNoiseLite n;
        n.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        n.SetSeed(4242);
        n.SetFrequency(0.0011f);
        n.SetFractalType(FastNoiseLite::FractalType_FBm);
        n.SetFractalOctaves(3);
        n.SetFractalGain(0.35f);
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

float Biome::Humidity(float worldX, float worldZ)
{
    return HumidityNoise().GetNoise(worldX, worldZ);
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
    return SampleHeight(Temperature(worldX, worldZ), Humidity(worldX, worldZ),
        Continentalness(worldX, worldZ), DetailNoise().GetNoise(worldX, worldZ));
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
        .heightAmplitude = 5,
        .grassColor = glm::vec3(0.569f, 0.741f, 0.349f),
        .leafColor = glm::vec3(0.471f, 0.671f, 0.302f)
    };

    // Floor (base - amplitude) sits above sea level so inland plains stay dry;
    // coastlines still dip into the sea via the continentalness blend.
    s_Biomes[BiomeType::k_Plains] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .baseHeight = 75,
        .heightAmplitude = 10,
        .grassColor = glm::vec3(0.569f, 0.741f, 0.349f),
        .leafColor = glm::vec3(0.471f, 0.671f, 0.302f)
    };

    s_Biomes[BiomeType::k_Desert] = {
        .surface = Block::k_Sand,
        .subsurface = Block::k_Sand,
        .subsurfaceDepth = 4,
        .baseHeight = 72,
        .heightAmplitude = 8,
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

BiomeType Biome::Select(float temperature, float humidity, float continentalness)
{
    if (Landness(continentalness) < 0.5f) {
        return BiomeType::k_Ocean;
    }
    // The same weight that blends the terrain height also decides the surface
    // block, so the grass/sand edge always lines up with the height transition.
    if (Desertness(temperature, humidity) >= 0.5f) {
        return BiomeType::k_Desert;
    }
    return BiomeType::k_Plains;
}

// Smoothstep ramp on a single axis, clamped to [0, 1].
static float SmoothRamp(float value, float start, float end)
{
    float t = (value - start) / (end - start);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

float Biome::Desertness(float temperature, float humidity)
{
    // Desert occupies the hot, dry corner of climate space. Treating it as the
    // intersection of two independent axes (a soft rectangle, like Minecraft's
    // multi-noise parameter boxes) keeps the border off any single field's
    // contour line. The product is a smooth weight reused for height blending.
    float hot = SmoothRamp(temperature, -0.2f, 0.2f);
    float dry = SmoothRamp(-humidity, -0.2f, 0.2f);
    return hot * dry;
}

float Biome::Landness(float continentalness)
{
    return SmoothRamp(continentalness, -0.3f, 0.0f);
}

int32_t Biome::SampleHeight(float temperature, float humidity, float continentalness, float noiseValue)
{
    const Biome& ocean = Get(BiomeType::k_Ocean);
    const Biome& plains = Get(BiomeType::k_Plains);
    const Biome& desert = Get(BiomeType::k_Desert);

    // Land terrain blends plains into desert by the same hot/dry weight that
    // chooses the surface block.
    float landBlend = Desertness(temperature, humidity);
    float landBase = plains.baseHeight + (desert.baseHeight - plains.baseHeight) * landBlend;
    float landAmplitude = plains.heightAmplitude + (desert.heightAmplitude - plains.heightAmplitude) * landBlend;

    // Then ocean blends into that land as the coast rises out of the water.
    float land = Landness(continentalness);
    float baseHeight = ocean.baseHeight + (landBase - ocean.baseHeight) * land;
    float amplitude = ocean.heightAmplitude + (landAmplitude - ocean.heightAmplitude) * land;

    return (int32_t)(amplitude * noiseValue + baseHeight);
}

} // namespace Krafter
