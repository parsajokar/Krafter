#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "vulkan/vulkan.h"

#include "Krafter/Rendering/ChunkMesh.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/Fluid.h"

namespace Krafter {

namespace {

struct FaceQuad {
    glm::vec3 origin;
    glm::vec3 dx;
    glm::vec3 dy;
    glm::vec3 normal;
};

FaceQuad FaceGeometryOf(const glm::vec3& position, BlockFace face)
{
    switch (face) {
    case BlockFace::k_Front:
        return { position, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec3(-1, 0, 0) };
    case BlockFace::k_Back:
        return { position + glm::vec3(1, 0, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0) };
    case BlockFace::k_Left:
        return { position + glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, -1) };
    case BlockFace::k_Right:
        return { position + glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1) };
    case BlockFace::k_Bottom:
        return { position + glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0) };
    default:
        return { position + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) };
    }
}

constexpr float k_AoFactor[] = { 0.5f, 0.7f, 0.85f, 1.0f };

template <typename T>
T Quantize(float value, float scale, float lo, float hi)
{
    return static_cast<T>(std::lround(std::clamp(value * scale, lo, hi)));
}

ChunkVertex PackVertex(const glm::vec3& position, const glm::vec2& uv, const glm::vec3& normal,
    float waterDepth, const glm::vec3& tint, float skyLight, float blockLight)
{
    ChunkVertex vertex;
    for (int32_t i = 0; i < 3; i++) {
        vertex.position[i] = Quantize<uint16_t>(position[i], ChunkVertex::k_PositionScale, 0.0f, 65535.0f);
        vertex.normal[i] = Quantize<int8_t>(normal[i], 127.0f, -127.0f, 127.0f);
        vertex.tint[i] = Quantize<uint8_t>(tint[i], 255.0f, 0.0f, 255.0f);
    }
    vertex.waterDepth = Quantize<uint16_t>(waterDepth, 1.0f, 0.0f, 65535.0f);
    vertex.uv[0] = Quantize<uint16_t>(uv.x, 65535.0f, 0.0f, 65535.0f);
    vertex.uv[1] = Quantize<uint16_t>(uv.y, 65535.0f, 0.0f, 65535.0f);
    vertex.blockLight = Quantize<int8_t>(blockLight, 127.0f, 0.0f, 127.0f);
    vertex.skyLight = Quantize<uint8_t>(skyLight, 255.0f, 0.0f, 255.0f);
    return vertex;
}

ChunkMeshBuffer& PassBuffer(ChunkMeshData& data, ChunkPass pass)
{
    return data.passes[static_cast<size_t>(pass)];
}

}

bool ChunkMeshData::IsEmpty() const
{
    for (const ChunkMeshBuffer& buffer : passes) {
        if (!buffer.elements.empty()) {
            return false;
        }
    }
    return true;
}

ChunkMeshData ChunkMesh::Compute(
    const std::array<const Chunk*, 9>& grid,
    const glm::ivec2& chunkPosition,
    const Climate& climate)
{
    ChunkMeshData data;

    constexpr int32_t dx[] = { -1, 1, 0, 0, 0, 0 };
    constexpr int32_t dy[] = { 0, 0, -1, 1, 0, 0 };
    constexpr int32_t dz[] = { 0, 0, 0, 0, -1, 1 };
    constexpr BlockFace faces[] = {
        BlockFace::k_Front, BlockFace::k_Back,
        BlockFace::k_Bottom, BlockFace::k_Top,
        BlockFace::k_Left, BlockFace::k_Right
    };

    constexpr glm::ivec3 faceU[] = {
        { 0, 0, 1 }, { 0, 0, -1 }, { 0, 0, 1 }, { 0, 0, 1 }, { -1, 0, 0 }, { 1, 0, 0 }
    };
    constexpr glm::ivec3 faceV[] = {
        { 0, 1, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 1, 0 }
    };

    auto resolve = [&](const glm::ivec3& cell, glm::ivec3& query) -> const Chunk* {
        const int32_t cx = FloorDiv(cell.x, Chunk::k_Width);
        const int32_t cz = FloorDiv(cell.z, Chunk::k_Width);
        if (cx < -1 || cx > 1 || cz < -1 || cz > 1) {
            return nullptr;
        }
        query = glm::ivec3(FloorMod(cell.x, Chunk::k_Width), cell.y, FloorMod(cell.z, Chunk::k_Width));
        return grid[(cz + 1) * 3 + (cx + 1)];
    };

    auto blockAt = [&](const glm::ivec3& cell) -> Block {
        if (cell.y < 0 || cell.y >= Chunk::k_Height) {
            return Block::k_Air;
        }
        glm::ivec3 query;
        const Chunk* target = resolve(cell, query);
        return target ? target->GetBlock(query) : Block::k_Air;
    };

    auto fluidAt = [&](const glm::ivec3& cell) -> uint8_t {
        if (cell.y < 0 || cell.y >= Chunk::k_Height) {
            return 0;
        }
        glm::ivec3 query;
        const Chunk* target = resolve(cell, query);
        return target ? target->GetFluid(query) : 0;
    };

    auto isSolid = [&](const glm::ivec3& cell) -> bool {
        const Block block = blockAt(cell);
        return IsOpaque(block) && !IsCutout(block);
    };

    auto hidesFace = [](Block self, Block neighbor) -> bool {
        if (self == Block::k_Cactus && neighbor == Block::k_Cactus) {
            return true;
        }
        if (IsCutout(neighbor)) {
            return !IsOpaque(self);
        }
        if (IsTranslucent(neighbor)) {
            return neighbor == self;
        }
        if (IsOpaque(neighbor)) {
            return true;
        }
        return neighbor == self;
    };

    auto skyLightOf = [&](const glm::ivec3& cell) -> float {
        if (cell.y >= Chunk::k_Height) {
            return 1.0f;
        }
        if (cell.y < 0) {
            return 0.0f;
        }
        glm::ivec3 query;
        const Chunk* target = resolve(cell, query);
        return target ? target->GetSkyLight(query) / static_cast<float>(Chunk::k_MaxLight) : 0.0f;
    };

    auto blockLightOf = [&](const glm::ivec3& cell) -> float {
        if (cell.y >= Chunk::k_Height || cell.y < 0) {
            return 0.0f;
        }
        glm::ivec3 query;
        const Chunk* target = resolve(cell, query);
        return target ? target->GetBlockLight(query) / static_cast<float>(Chunk::k_MaxLight) : 0.0f;
    };

    auto cornerLight = [&](auto&& sample, const glm::ivec3& airCell, const glm::ivec3& u, const glm::ivec3& v,
                           int32_t su, int32_t sv) -> float {
        const glm::ivec3 sideU = airCell + su * u;
        const glm::ivec3 sideV = airCell + sv * v;
        const glm::ivec3 corner = airCell + su * u + sv * v;

        const bool s1 = isSolid(sideU);
        const bool s2 = isSolid(sideV);
        const bool sc = isSolid(corner);

        const int32_t ao = (s1 && s2) ? 0 : 3 - (static_cast<int32_t>(s1) + static_cast<int32_t>(s2) + static_cast<int32_t>(sc));

        float sum = sample(airCell);
        int32_t count = 1;
        if (!s1) {
            sum += sample(sideU);
            count++;
        }
        if (!s2) {
            sum += sample(sideV);
            count++;
        }
        if (!sc && !(s1 && s2)) {
            sum += sample(corner);
            count++;
        }

        return (sum / count) * k_AoFactor[ao];
    };

    const Chunk& center = *grid[4];

    auto waterDepth = [&](int32_t lx, int32_t y, int32_t lz) -> float {
        int32_t top = y;
        while (top + 1 < Chunk::k_Height && center.GetBlock(glm::ivec3(lx, top + 1, lz)) == Block::k_Water) {
            top++;
        }
        int32_t bottom = y;
        while (bottom - 1 >= 0 && center.GetBlock(glm::ivec3(lx, bottom - 1, lz)) == Block::k_Water) {
            bottom--;
        }
        return static_cast<float>(top - bottom + 1);
    };

    const int32_t top = center.GetTopY();
    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t y = 0; y < top; y++) {
            for (int32_t z = 0; z < Chunk::k_Width; z++) {
                const Block self = center.GetBlock(glm::ivec3(x, y, z));
                if (self == Block::k_Air) {
                    continue;
                }

                const glm::vec3 localPos(x, y, z);

                if (IsPlant(self) || IsGem(self)) {
                    glm::vec3 tint(1.0f);
                    if (self == Block::k_ShortGrass || self == Block::k_Fern) {
                        const float worldX = static_cast<float>(chunkPosition.x * Chunk::k_Width + x);
                        const float worldZ = static_cast<float>(chunkPosition.y * Chunk::k_Width + z);
                        tint = Biome::Get(climate.BiomeAt(worldX, worldZ)).grassColor;
                    }
                    const float light = skyLightOf(glm::ivec3(x, y, z));
                    const float blockLight = blockLightOf(glm::ivec3(x, y, z));
                    const glm::vec2 tile = BlockAtlas::GetAtlasOf(self).side;

                    glm::ivec3 growth(0, 1, 0);
                    if (IsGem(self)) {
                        const glm::ivec3 cell(x, y, z);
                        if (isSolid(cell + glm::ivec3(0, -1, 0))) {
                            growth = glm::ivec3(0, 1, 0);
                        } else if (isSolid(cell + glm::ivec3(0, 1, 0))) {
                            growth = glm::ivec3(0, -1, 0);
                        } else if (isSolid(cell + glm::ivec3(-1, 0, 0))) {
                            growth = glm::ivec3(1, 0, 0);
                        } else if (isSolid(cell + glm::ivec3(1, 0, 0))) {
                            growth = glm::ivec3(-1, 0, 0);
                        } else if (isSolid(cell + glm::ivec3(0, 0, -1))) {
                            growth = glm::ivec3(0, 0, 1);
                        } else if (isSolid(cell + glm::ivec3(0, 0, 1))) {
                            growth = glm::ivec3(0, 0, -1);
                        }
                    }
                    AddCrossToData(localPos, tile, tint, light, blockLight, PassBuffer(data, ChunkPass::k_Cutout), growth);
                    continue;
                }

                const bool isFluid = IsFluid(self);
                const bool transparent = self == Block::k_Water;
                const bool translucent = IsTranslucent(self);
                ChunkMeshBuffer& buffer = PassBuffer(data,
                    translucent       ? ChunkPass::k_Translucent
                        : transparent ? ChunkPass::k_Transparent
                                      : ChunkPass::k_Opaque);

                float topInset = 0.0f;
                float depth = 0.0f;
                float cornerHeights[4] = {};
                if (isFluid) {
                    const uint8_t fluidByte = center.GetFluid(glm::ivec3(x, y, z));
                    const bool fluidAbove = blockAt(glm::ivec3(x, y + 1, z)) == self;
                    topInset = fluidAbove ? 0.0f : 1.0f - FluidHeight(fluidByte);
                    if (transparent) {
                        depth = waterDepth(x, y, z);
                    }

                    for (int32_t rx = 0; rx <= 1; rx++) {
                        for (int32_t rz = 0; rz <= 1; rz++) {
                            float sum = 0.0f;
                            int32_t count = 0;
                            bool full = false;
                            for (int32_t ox = rx - 1; ox <= rx; ox++) {
                                for (int32_t oz = rz - 1; oz <= rz; oz++) {
                                    const glm::ivec3 c(x + ox, y, z + oz);
                                    if (blockAt(c) != self) {
                                        continue;
                                    }
                                    if (blockAt(glm::ivec3(c.x, y + 1, c.z)) == self) {
                                        full = true;
                                    }
                                    sum += FluidHeight(fluidAt(c));
                                    count++;
                                }
                            }
                            cornerHeights[rx * 2 + rz] = full ? static_cast<float>(y) + 1.0f
                                                              : static_cast<float>(y) + (count > 0 ? sum / count : FluidHeight(fluidByte));
                        }
                    }
                }

                glm::vec3 grassTint(1.0f);
                glm::vec3 leafTint(1.0f);
                if (self == Block::k_Grass || IsLeaves(self)) {
                    const float worldX = static_cast<float>(chunkPosition.x * Chunk::k_Width + x);
                    const float worldZ = static_cast<float>(chunkPosition.y * Chunk::k_Width + z);
                    const Biome& biome = Biome::Get(climate.BiomeAt(worldX, worldZ));
                    grassTint = biome.grassColor;
                    leafTint = biome.leafColor;
                }

                for (size_t k = 0; k < 6; k++) {
                    glm::ivec3 airCell(x + dx[k], y + dy[k], z + dz[k]);
                    if (airCell.y < 0) {
                        continue;
                    }
                    const Block neighbor = blockAt(airCell);
                    if (hidesFace(self, neighbor)) {
                        continue;
                    }

                    std::array<float, 4> vertexLight = {
                        cornerLight(skyLightOf, airCell, faceU[k], faceV[k], -1, -1),
                        cornerLight(skyLightOf, airCell, faceU[k], faceV[k], 1, -1),
                        cornerLight(skyLightOf, airCell, faceU[k], faceV[k], 1, 1),
                        cornerLight(skyLightOf, airCell, faceU[k], faceV[k], -1, 1)
                    };
                    std::array<float, 4> vertexBlockLight = {
                        cornerLight(blockLightOf, airCell, faceU[k], faceV[k], -1, -1),
                        cornerLight(blockLightOf, airCell, faceU[k], faceV[k], 1, -1),
                        cornerLight(blockLightOf, airCell, faceU[k], faceV[k], 1, 1),
                        cornerLight(blockLightOf, airCell, faceU[k], faceV[k], -1, 1)
                    };

                    if (LightEmission(self) > 0) {
                        const float glow = LightEmission(self) / static_cast<float>(Chunk::k_MaxLight);
                        for (float& corner : vertexBlockLight) {
                            corner = std::max(corner, glow);
                        }
                    }

                    const bool grassTop = self == Block::k_Grass && faces[k] == BlockFace::k_Top;
                    glm::vec3 tint(1.0f);
                    if (grassTop) {
                        tint = grassTint;
                    } else if (IsLeaves(self)) {
                        tint = leafTint;
                    }
                    AddFaceToData(localPos, self, faces[k], topInset, depth, tint, vertexLight, vertexBlockLight, buffer, nullptr, k_NoFlow, isFluid ? cornerHeights : nullptr);

                    const bool grassSide = self == Block::k_Grass
                        && faces[k] != BlockFace::k_Top && faces[k] != BlockFace::k_Bottom;
                    if (grassSide) {
                        const glm::vec2 overlay = BlockAtlas::GetAtlasOf(self).sideOverlay;
                        AddOverlayFace(localPos, faces[k], overlay, grassTint, vertexLight, vertexBlockLight, buffer);
                    }
                }
            }
        }
    }

    bool any = false;
    for (const ChunkMeshBuffer& buffer : data.passes) {
        for (const ChunkVertex& vertex : buffer.vertices) {
            const float y = vertex.position[1] / ChunkVertex::k_PositionScale;
            data.minY = any ? std::min(data.minY, y) : y;
            data.maxY = any ? std::max(data.maxY, y) : y;
            any = true;
        }
    }

    return data;
}

ChunkMesh::ChunkMesh(const ChunkMeshData& data, GeometryArena& arena)
    : m_Arena(arena)
    , m_MinY(data.minY)
    , m_MaxY(data.maxY)
{
    if (data.IsEmpty()) {
        return;
    }

    size_t vertexCount = 0;
    size_t indexCount = 0;
    for (const ChunkMeshBuffer& buffer : data.passes) {
        vertexCount += buffer.vertices.size();
        indexCount += buffer.elements.size();
    }

    std::vector<uint8_t> bytes(vertexCount * sizeof(ChunkVertex) + indexCount * sizeof(uint32_t));
    size_t vertexCursor = 0;
    size_t indexCursor = 0;
    for (const ChunkMeshBuffer& buffer : data.passes) {
        std::memcpy(bytes.data() + vertexCursor * sizeof(ChunkVertex),
            buffer.vertices.data(), buffer.vertices.size() * sizeof(ChunkVertex));
        std::memcpy(bytes.data() + vertexCount * sizeof(ChunkVertex) + indexCursor * sizeof(uint32_t),
            buffer.elements.data(), buffer.elements.size() * sizeof(uint32_t));
        vertexCursor += buffer.vertices.size();
        indexCursor += buffer.elements.size();
    }

    // Vertices must start on a whole-vertex offset, but VMA only aligns to powers of two.
    constexpr VkDeviceSize k_Stride = sizeof(ChunkVertex);
    m_Allocation = arena.Allocate(bytes.size() + k_Stride, alignof(uint32_t));
    const VkDeviceSize start = (m_Allocation.offset + k_Stride - 1) / k_Stride * k_Stride;
    arena.Upload(m_Allocation, start - m_Allocation.offset, bytes.data(), bytes.size());

    const uint32_t baseVertex = static_cast<uint32_t>(start / k_Stride);
    const uint32_t baseIndex = static_cast<uint32_t>((start + vertexCount * k_Stride) / sizeof(uint32_t));
    vertexCursor = 0;
    indexCursor = 0;
    for (size_t pass = 0; pass < k_ChunkPassCount; pass++) {
        const ChunkMeshBuffer& buffer = data.passes[pass];
        m_Parts[pass].indexCount = static_cast<uint32_t>(buffer.elements.size());
        m_Parts[pass].firstIndex = baseIndex + static_cast<uint32_t>(indexCursor);
        m_Parts[pass].vertexOffset = static_cast<int32_t>(baseVertex + vertexCursor);
        vertexCursor += buffer.vertices.size();
        indexCursor += buffer.elements.size();
    }
}

ChunkMesh::~ChunkMesh()
{
    m_Arena.Free(m_Allocation);
}

void ChunkMesh::AddFaceToData(
    const std::array<glm::vec3, 4>& positionList,
    const std::array<glm::vec2, 2>& uvCoordsList,
    const glm::vec3& normal, float waterDepth, const glm::vec3& tint,
    const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
    ChunkMeshBuffer& buffer,
    const std::array<glm::vec2, 4>* explicitUvs)
{
    std::vector<ChunkVertex>& vertexBufferData = buffer.vertices;
    std::vector<uint32_t>& elementBufferData = buffer.elements;
    const uint32_t offset = static_cast<uint32_t>(vertexBufferData.size());

    const std::array<glm::vec2, 4> uvs = explicitUvs ? *explicitUvs
                                                     : std::array<glm::vec2, 4> {
                                                           glm::vec2(uvCoordsList[0].x, uvCoordsList[0].y),
                                                           glm::vec2(uvCoordsList[1].x, uvCoordsList[0].y),
                                                           glm::vec2(uvCoordsList[1].x, uvCoordsList[1].y),
                                                           glm::vec2(uvCoordsList[0].x, uvCoordsList[1].y)
                                                       };

    for (size_t i = 0; i < 4; i++) {
        vertexBufferData.push_back(PackVertex(
            positionList[i], uvs[i], normal, waterDepth, tint, vertexLight[i], vertexBlockLight[i]));
    }

    if (vertexLight[0] + vertexLight[2] < vertexLight[1] + vertexLight[3]) {
        elementBufferData.push_back(offset + 1);
        elementBufferData.push_back(offset + 3);
        elementBufferData.push_back(offset + 0);

        elementBufferData.push_back(offset + 1);
        elementBufferData.push_back(offset + 2);
        elementBufferData.push_back(offset + 3);
    } else {
        elementBufferData.push_back(offset + 0);
        elementBufferData.push_back(offset + 1);
        elementBufferData.push_back(offset + 2);

        elementBufferData.push_back(offset + 0);
        elementBufferData.push_back(offset + 2);
        elementBufferData.push_back(offset + 3);
    }
}

void ChunkMesh::AddFaceToData(
    const glm::vec3& position,
    const Block block, BlockFace face, float topInset, float waterDepth, const glm::vec3& tint,
    const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
    ChunkMeshBuffer& buffer,
    const glm::vec2* tileOverride, float flowAngle, const float* cornerHeights)
{
    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);
    glm::vec2 uvCoords = atlas.side;
    if (face == BlockFace::k_Top) {
        uvCoords = atlas.top;
    } else if (face == BlockFace::k_Bottom) {
        uvCoords = atlas.bottom;
    } else if (face == BlockFace::k_Front
        && (block == Block::k_Workbench || block == Block::k_Furnace)) {
        uvCoords = atlas.front;
    }
    if (tileOverride) {
        uvCoords = *tileOverride;
    }

    const FaceQuad quad = FaceGeometryOf(position, face);
    std::array<glm::vec3, 4> positionList = {
        quad.origin, quad.origin + quad.dx, quad.origin + quad.dx + quad.dy, quad.origin + quad.dy
    };

    if (block == Block::k_Cactus && face != BlockFace::k_Top && face != BlockFace::k_Bottom) {
        const glm::vec3 inset = quad.normal * (1.0f / 16.0f);
        for (glm::vec3& vertex : positionList) {
            vertex -= inset;
        }
    }

    if (cornerHeights) {
        for (glm::vec3& vertex : positionList) {
            if (vertex.y > position.y + 0.5f) {
                const int32_t rx = static_cast<int32_t>(std::lround(vertex.x - position.x));
                const int32_t rz = static_cast<int32_t>(std::lround(vertex.z - position.z));
                vertex.y = cornerHeights[rx * 2 + rz];
            }
        }
    } else if (topInset > 0.0f) {
        for (glm::vec3& vertex : positionList) {
            if (vertex.y > position.y + 0.5f) {
                vertex.y -= topInset;
            }
        }
    }

    const std::array<glm::vec2, 2> uvCoordsList = { uvCoords, uvCoords + glm::vec2(BlockAtlas::k_Step) };

    if (flowAngle != k_NoFlow) {
        const glm::vec2 center = uvCoords + glm::vec2(BlockAtlas::k_Step * 0.5f);
        const float c = std::cos(flowAngle);
        const float s = std::sin(flowAngle);
        constexpr float scale = 0.70710678f * BlockAtlas::k_Step;
        const glm::vec2 base[4] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } };
        std::array<glm::vec2, 4> uvs;
        for (size_t i = 0; i < 4; i++) {
            uvs[i] = center + glm::vec2(base[i].x * c - base[i].y * s, base[i].x * s + base[i].y * c) * scale;
        }
        AddFaceToData(positionList, uvCoordsList, quad.normal, waterDepth, tint, vertexLight, vertexBlockLight, buffer, &uvs);
    } else {
        AddFaceToData(positionList, uvCoordsList, quad.normal, waterDepth, tint, vertexLight, vertexBlockLight, buffer);
    }
}

void ChunkMesh::AddOverlayFace(
    const glm::vec3& position, BlockFace face,
    const glm::vec2& tile, const glm::vec3& tint,
    const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
    ChunkMeshBuffer& buffer)
{
    const FaceQuad quad = FaceGeometryOf(position, face);
    const std::array<glm::vec3, 4> positionList = {
        quad.origin, quad.origin + quad.dx, quad.origin + quad.dx + quad.dy, quad.origin + quad.dy
    };

    const std::array<glm::vec2, 2> uvCoordsList = { tile, tile + glm::vec2(BlockAtlas::k_Step) };

    AddFaceToData(positionList, uvCoordsList, quad.normal, 0.0f, tint, vertexLight, vertexBlockLight, buffer);
}

void ChunkMesh::AddCrossToData(
    const glm::vec3& position, const glm::vec2& tile, const glm::vec3& tint, float light, float blockLight,
    ChunkMeshBuffer& buffer,
    const glm::ivec3& growth)
{
    const std::array<float, 4> vertexLight = { light, light, light, light };
    const std::array<float, 4> vertexBlockLight = { blockLight, blockLight, blockLight, blockLight };
    const std::array<glm::vec2, 2> uvCoordsList = { tile, tile + glm::vec2(BlockAtlas::k_Step) };

    const glm::vec3 gAxis(growth);
    glm::vec3 uAxis;
    glm::vec3 wAxis;
    if (growth.y != 0) {
        uAxis = glm::vec3(1, 0, 0);
        wAxis = glm::vec3(0, 0, 1);
    } else if (growth.x != 0) {
        uAxis = glm::vec3(0, 1, 0);
        wAxis = glm::vec3(0, 0, 1);
    } else {
        uAxis = glm::vec3(1, 0, 0);
        wAxis = glm::vec3(0, 1, 0);
    }

    const glm::vec3 center = position + glm::vec3(0.5f);
    const glm::vec3 normal = gAxis;
    auto vertex = [&](float g, float u, float w) {
        return center + g * gAxis + u * uAxis + w * wAxis;
    };

    const std::array<glm::vec3, 4> plane1 = {
        vertex(-0.5f, -0.5f, -0.5f), vertex(-0.5f, 0.5f, 0.5f),
        vertex(0.5f, 0.5f, 0.5f), vertex(0.5f, -0.5f, -0.5f)
    };
    AddFaceToData(plane1, uvCoordsList, normal, 0.0f, tint, vertexLight, vertexBlockLight, buffer);

    const std::array<glm::vec3, 4> plane2 = {
        vertex(-0.5f, 0.5f, -0.5f), vertex(-0.5f, -0.5f, 0.5f),
        vertex(0.5f, -0.5f, 0.5f), vertex(0.5f, 0.5f, -0.5f)
    };
    AddFaceToData(plane2, uvCoordsList, normal, 0.0f, tint, vertexLight, vertexBlockLight, buffer);
}

}
