#pragma once

#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "FastNoiseLite.h"

#include "Krafter/World/Block.h"

namespace Krafter {

class Chunk;

class TerrainGenerator {
public:
    explicit TerrainGenerator(int32_t seed);

    void Generate(Chunk& chunk, const glm::ivec2& position) const;

private:
    struct Ellipsoid {
        float ex, ey, ez;
        float rx, ry, rz;
    };

    struct Lake {
        int32_t cy = 0;
        int32_t minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
        std::vector<Ellipsoid> blobs;

        bool Contains(int32_t x, int32_t y, int32_t z) const;
        float SurfaceField(int32_t x, int32_t z) const;
    };

    struct Tree {
        int32_t x = 0, baseY = 0, z = 0;
        int32_t trunkHeight = 0;
        uint32_t seed = 0;
        Block wood = Block::k_OakWood;
        Block leaves = Block::k_OakLeaves;
    };

    uint32_t Hash(int32_t x, int32_t z, uint32_t salt) const;
    float Hash01(int32_t x, int32_t z, uint32_t salt) const;

    int32_t AquiferCellLevel(int32_t gx, int32_t gz, float cx, float cz) const;
    Block AquiferBlock(int32_t worldX, int32_t y, int32_t worldZ, int32_t surface, int32_t surfaceMargin) const;

    bool BuildLake(int32_t cellX, int32_t cellZ, Lake& lake) const;
    bool LakeContained(const Lake& lake) const;
    std::vector<Lake> GatherLakes(const glm::ivec2& chunkPosition) const;
    void CarveLake(Chunk& chunk, const glm::ivec2& chunkPosition, const Lake& lake) const;
    bool ColumnInLake(int32_t worldX, int32_t worldZ) const;

    bool BuildTree(int32_t cellX, int32_t cellZ, Tree& tree) const;
    std::vector<Tree> GatherTrees(const glm::ivec2& chunkPosition) const;
    void PlaceTreeBlock(Chunk& chunk, const glm::ivec2& chunkPosition, int32_t worldX, int32_t y, int32_t worldZ, Block block) const;
    void StampRootFlare(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree) const;
    void StampAcacia(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree) const;
    void StampLeafBlob(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree,
        int32_t cx, int32_t cz, int32_t cy, uint32_t salt) const;
    void StampTree(Chunk& chunk, const glm::ivec2& chunkPosition, const Tree& tree) const;

    bool ColumnRollsCactus(int32_t worldX, int32_t worldZ) const;
    void ScatterPlants(Chunk& chunk, const glm::ivec2& chunkPosition) const;

    void CarveCaves(Chunk& chunk, const glm::ivec2& chunkPosition) const;

    uint32_t m_Seed = 0;

    FastNoiseLite m_CaveNoiseA;
    FastNoiseLite m_CaveNoiseB;
    FastNoiseLite m_CavernNoise;
    FastNoiseLite m_AquiferLevelNoise;
    FastNoiseLite m_AquiferMaskNoise;
    FastNoiseLite m_EntranceNoise;
};

}
