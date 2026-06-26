#include <iostream>
#include <stdexcept>

#include "FastNoiseLite.h"

#include "Krafter/World/Biome.h"

namespace Krafter {

// The climate and detail fields, configured once by Biome::Configure before any
// chunk is generated. FastNoiseLite::GetNoise is const, so the chunk workers and
// the main thread can read them concurrently afterwards.
//
// Climate fields are fractal (FBm) rather than a single smooth octave. A single
// octave has near-straight contour lines, so thresholding it for a biome border
// produces unnaturally straight edges; stacking octaves wrinkles the contour at
// every scale, the way Minecraft's multi-noise climate does.
static FastNoiseLite s_TemperatureNoise;
static FastNoiseLite s_HumidityNoise;
static FastNoiseLite s_ContinentNoise;
static FastNoiseLite s_DetailNoise;

void Biome::Configure(int32_t seed)
{
    // Each field offsets the shared seed by its own constant, so one seed drives
    // all of them while keeping the fields independent. Seed 0 reproduces the
    // original fixed world.
    s_TemperatureNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_TemperatureNoise.SetSeed(1337 + seed);
    // Low base frequency keeps biomes large; the threshold is crossed on this
    // scale. The few low-gain octaves only ride on top to wrinkle the border
    // without spawning small islands of the other biome.
    s_TemperatureNoise.SetFrequency(0.0009f);
    s_TemperatureNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    s_TemperatureNoise.SetFractalOctaves(3);
    s_TemperatureNoise.SetFractalGain(0.35f);

    // Second, independent climate axis. Desert is the hot *and* dry corner of the
    // (temperature, humidity) plane, so the border no longer follows the contour
    // of a single field — it's where two unrelated noise fields jointly cross out
    // of the desert box, which is far less regular than either field alone.
    s_HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_HumidityNoise.SetSeed(4242 + seed);
    s_HumidityNoise.SetFrequency(0.0011f);
    s_HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    s_HumidityNoise.SetFractalOctaves(3);
    s_HumidityNoise.SetFractalGain(0.35f);

    s_ContinentNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_ContinentNoise.SetSeed(2024 + seed);
    s_ContinentNoise.SetFrequency(0.0008f);

    // Fine-grained terrain detail driving the per-column height variation. Its
    // default seed is FastNoiseLite's own 1337, kept here so seed 0 is unchanged.
    s_DetailNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    s_DetailNoise.SetSeed(1337 + seed);
    s_DetailNoise.SetFrequency(0.02f);

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

int32_t Biome::SurfaceHeight(float worldX, float worldZ)
{
    return SampleHeight(Temperature(worldX, worldZ), Humidity(worldX, worldZ),
        Continentalness(worldX, worldZ), s_DetailNoise.GetNoise(worldX, worldZ));
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
        // Well below sea level, so ocean columns flood into deep water.
        .baseHeight = 40,
        .heightAmplitude = 5,
        .grassColor = glm::vec3(0.569f, 0.741f, 0.349f),
        .leafColor = glm::vec3(0.471f, 0.671f, 0.302f)
    };

    // Floor (base - amplitude) sits above sea level so inland forest stays dry;
    // coastlines still dip into the sea via the continentalness blend.
    s_Biomes[BiomeType::k_OakForest] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .baseHeight = 75,
        .heightAmplitude = 10,
        .grassColor = glm::vec3(0.569f, 0.741f, 0.349f),
        .leafColor = glm::vec3(0.471f, 0.671f, 0.302f)
    };

    // Cooler, drier temperate forest: a paler, slightly yellow-green to match
    // the brighter birch foliage.
    s_Biomes[BiomeType::k_BirchForest] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .baseHeight = 75,
        .heightAmplitude = 10,
        .grassColor = glm::vec3(0.624f, 0.769f, 0.412f),
        .leafColor = glm::vec3(0.553f, 0.722f, 0.376f)
    };

    // Warm, arid grassland dotted with acacia: dry olive grass and foliage,
    // greener than the desert but well short of the lush forests.
    s_Biomes[BiomeType::k_Savannah] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4,
        .baseHeight = 73,
        .heightAmplitude = 8,
        .grassColor = glm::vec3(0.741f, 0.722f, 0.357f),
        .leafColor = glm::vec3(0.643f, 0.639f, 0.318f)
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

// Smoothstep ramp on a single axis, clamped to [0, 1].
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

    // Land is split along two independent climate axes (the same hot/dry axes
    // that Desertness multiplies, so the desert here still coincides with the
    // height blend). Temperature separates the cool forests from the hot
    // grasslands; within each band, humidity picks the drier of the pair:
    //
    //          dry            wet
    //   hot    Desert         Savannah
    //   cool   Birch Forest   Oak Forest
    const bool hot = SmoothRamp(temperature, -0.2f, 0.2f) >= 0.5f;
    const bool dry = SmoothRamp(-humidity, -0.2f, 0.2f) >= 0.5f;

    if (hot) {
        return dry ? BiomeType::k_Desert : BiomeType::k_Savannah;
    }
    return dry ? BiomeType::k_BirchForest : BiomeType::k_OakForest;
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
    const Biome& oakForest = Get(BiomeType::k_OakForest);
    const Biome& desert = Get(BiomeType::k_Desert);

    // Land terrain blends oak forest into desert by the same hot/dry weight that
    // chooses the surface block.
    float landBlend = Desertness(temperature, humidity);
    float landBase = oakForest.baseHeight + (desert.baseHeight - oakForest.baseHeight) * landBlend;
    float landAmplitude = oakForest.heightAmplitude + (desert.heightAmplitude - oakForest.heightAmplitude) * landBlend;

    // Then ocean blends into that land as the coast rises out of the water.
    float land = Landness(continentalness);
    float baseHeight = ocean.baseHeight + (landBase - ocean.baseHeight) * land;
    float amplitude = ocean.heightAmplitude + (landAmplitude - ocean.heightAmplitude) * land;

    return (int32_t)(amplitude * noiseValue + baseHeight);
}

} // namespace Krafter
