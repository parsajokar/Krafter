#include <algorithm>
#include <array>
#include <cstdint>

#include "vulkan/vulkan.h"

#include "Krafter/Core/Renderer.h"
#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/Coords.h"

namespace Krafter {

// Origin and in-plane axes of a block face, shared by the regular face builder
// and the grass overlay. Vertices wind origin -> +dx -> +dx+dy -> +dy, which is
// counter-clockwise seen from outside (matching the back-face culling).
struct FaceQuad {
    glm::vec3 origin;
    glm::vec3 dx;
    glm::vec3 dy;
    glm::vec3 normal;
};

static FaceQuad FaceGeometryOf(const glm::vec3& position, BlockFace face)
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
    default: // k_Top
        return { position + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0) };
    }
}

// Ambient-occlusion brightness for the four corner levels (0 = most occluded).
static constexpr float k_AoFactor[] = { 0.5f, 0.7f, 0.85f, 1.0f };

// Floats per vertex: position(3), uv(2), normal(3), sky light(1), water depth(1),
// tint(3), block light(1).
static constexpr int32_t k_VertexStride = 14;

ChunkMeshData ChunkMesh::Compute(
    const std::array<const Chunk*, 9>& grid,
    const glm::ivec2& chunkPosition)
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

    // In-plane axes of each face, matching the quad's dx/dy in AddFaceToData.
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

    // Ambient occlusion and smooth lighting share the sky-light pass's notion of
    // a solid occluder: a full opaque cube. Air, water, and cutout foliage (leaves
    // and cactus) let their neighbours' faces through, so a cactus doesn't shade
    // the sand it stands on the way a real block would.
    auto isSolid = [&](const glm::ivec3& cell) -> bool {
        const Block block = blockAt(cell);
        return IsOpaque(block) && !IsCutout(block);
    };

    // Whether `neighbor` fully covers the shared face of `self`, letting us cull
    // it. Opaque blocks hide anything behind them. Cutout foliage has holes, so
    // to other solid geometry it never hides a neighbour: the dirt under a leaf
    // still draws its top, and (like Minecraft's "fancy" leaves) the shared faces
    // between adjacent leaf blocks are kept, so a canopy reads as dense layers.
    // To water, though, a leaf reads as solid, so a submerged water surface is
    // culled against it rather than z-fighting the coplanar leaf face. Water
    // otherwise only culls against more water, so water-vs-air surfaces show.
    auto hidesFace = [](Block self, Block neighbor) -> bool {
        // Stacked cactus culls its shared cap/base faces against itself, so the
        // coplanar segment boundary doesn't draw two z-fighting faces.
        if (self == Block::k_Cactus && neighbor == Block::k_Cactus) {
            return true;
        }
        if (IsCutout(neighbor)) {
            return !IsOpaque(self);
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

    // Block light has no source above the world, so unlike sky it is zero off the
    // top of the column rather than full.
    auto blockLightOf = [&](const glm::ivec3& cell) -> float {
        if (cell.y >= Chunk::k_Height || cell.y < 0) {
            return 0.0f;
        }
        glm::ivec3 query;
        const Chunk* target = resolve(cell, query);
        return target ? target->GetBlockLight(query) / static_cast<float>(Chunk::k_MaxLight) : 0.0f;
    };

    // Smooth light + ambient occlusion for the face corner pointing toward
    // (su, sv), sampling the two side cells and the diagonal on the air side.
    // `sample` selects the light channel (sky or block), so both are gathered the
    // same way, with the same AO darkening.
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
        // Two solid edges seal off the diagonal, so its light can't reach this
        // vertex; sampling it anyway is what leaks light through missing corners.
        if (!sc && !(s1 && s2)) {
            sum += sample(corner);
            count++;
        }

        return (sum / count) * k_AoFactor[ao];
    };

    const Chunk& center = *grid[4];

    // Thickness of the contiguous water column containing this cell, so the
    // shader can fade water from clear in the shallows to opaque in deep water.
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

    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t y = 0; y < Chunk::k_Height; y++) {
            for (int32_t z = 0; z < Chunk::k_Width; z++) {
                const Block self = center.GetBlock(glm::ivec3(x, y, z));
                if (self == Block::k_Air) {
                    continue;
                }

                glm::vec3 worldPos(
                    chunkPosition.x * Chunk::k_Width + x,
                    y,
                    chunkPosition.y * Chunk::k_Width + z);

                // Cross plants short-circuit the per-face cube path: they are two
                // crossed billboards, lit flatly from their own cell's sky light
                // (no AO, since they cast none). Grass and ferns are grayscale and
                // biome-tinted; the dead bush keeps its own brown (untinted).
                if (IsPlant(self)) {
                    glm::vec3 tint(1.0f);
                    if (self != Block::k_DeadBush && self != Block::k_Torch) {
                        const float worldX = static_cast<float>(chunkPosition.x * Chunk::k_Width + x);
                        const float worldZ = static_cast<float>(chunkPosition.y * Chunk::k_Width + z);
                        tint = Biome::Get(Biome::At(worldX, worldZ)).grassColor;
                    }
                    const float light = skyLightOf(glm::ivec3(x, y, z));
                    const float blockLight = blockLightOf(glm::ivec3(x, y, z));
                    const glm::vec2 tile = BlockAtlas::GetAtlasOf(self).side;
                    AddCrossToData(worldPos, tile, tint, light, blockLight, data.cross.vertices, data.cross.elements);
                    continue;
                }

                const bool transparent = self == Block::k_Water;
                ChunkMeshBuffer& buffer = transparent ? data.transparent : data.opaque;

                // Water that isn't submerged under more water sits below a full
                // block, so its surface reads as sunken. This holds even when a
                // solid block covers it: the inset leaves a visible gap.
                const bool surfaceWater = transparent && blockAt(glm::ivec3(x, y + 1, z)) != Block::k_Water;
                const float topInset = surfaceWater ? 4.0f / 16.0f : 0.0f;

                const float depth = transparent ? waterDepth(x, y, z) : 0.0f;

                // Grass tops/fringes and leaves are grayscale; tint them by the
                // biome at this column. Grass tints only its top and side fringe;
                // leaves tint every face. Everything else stays untinted (white).
                glm::vec3 grassTint(1.0f);
                glm::vec3 leafTint(1.0f);
                if (self == Block::k_Grass || IsLeaves(self)) {
                    const float worldX = static_cast<float>(chunkPosition.x * Chunk::k_Width + x);
                    const float worldZ = static_cast<float>(chunkPosition.y * Chunk::k_Width + z);
                    const Biome& biome = Biome::Get(Biome::At(worldX, worldZ));
                    grassTint = biome.grassColor;
                    leafTint = biome.leafColor;
                }

                for (size_t k = 0; k < 6; k++) {
                    glm::ivec3 airCell(x + dx[k], y + dy[k], z + dz[k]);
                    if (airCell.y < 0) {
                        continue;
                    }
                    const Block neighbor = blockAt(airCell);
                    if (transparent && faces[k] == BlockFace::k_Top) {
                        // The inset surface sits below whatever is above, so draw
                        // it unless submerged under more water.
                        if (neighbor == Block::k_Water) {
                            continue;
                        }
                    } else if (hidesFace(self, neighbor)) {
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

                    // An emissive block's own faces glow at its emission level even
                    // if the air in front is otherwise dark, so lava (and later
                    // torches) always read bright regardless of the flood.
                    if (LightEmission(self) > 0) {
                        const float glow = LightEmission(self) / static_cast<float>(Chunk::k_MaxLight);
                        for (float& corner : vertexBlockLight) {
                            corner = std::max(corner, glow);
                        }
                    }

                    // Only the grass top is the tinted gray tile; the side base
                    // and bottom are plain dirt and stay untinted. Leaves are
                    // grayscale on every face, so the whole block is tinted.
                    const bool grassTop = self == Block::k_Grass && faces[k] == BlockFace::k_Top;
                    glm::vec3 tint(1.0f);
                    if (grassTop) {
                        tint = grassTint;
                    } else if (IsLeaves(self)) {
                        tint = leafTint;
                    }
                    AddFaceToData(worldPos, self, faces[k], topInset, depth, tint, vertexLight, vertexBlockLight, buffer.vertices, buffer.elements);

                    // Grass side faces get the biome-tinted fringe layered on top
                    // of the dirt base.
                    const bool grassSide = self == Block::k_Grass
                        && faces[k] != BlockFace::k_Top && faces[k] != BlockFace::k_Bottom;
                    if (grassSide) {
                        const glm::vec2 overlay = BlockAtlas::GetAtlasOf(self).sideOverlay;
                        AddOverlayFace(worldPos, faces[k], overlay, grassTint, vertexLight, vertexBlockLight, buffer.vertices, buffer.elements);
                    }
                }
            }
        }
    }

    return data;
}

void ChunkMesh::Upload(Part& part, const ChunkMeshBuffer& buffer)
{
    part.elementCount = static_cast<uint32_t>(buffer.elements.size());
    if (part.elementCount == 0) {
        return;
    }

    // The interleaved vertex layout (k_VertexStride floats) is described by the
    // pipeline, not the buffer; here we only upload the raw vertex and index data.
    Renderer& renderer = Renderer::Get();
    part.vertexBuffer = renderer.CreateDeviceLocalBuffer(
        buffer.vertices.data(), buffer.vertices.size() * sizeof(float),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    part.indexBuffer = renderer.CreateDeviceLocalBuffer(
        buffer.elements.data(), buffer.elements.size() * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void ChunkMesh::Release(Part& part)
{
    if (part.elementCount == 0) {
        return;
    }
    Renderer& renderer = Renderer::Get();
    renderer.DestroyBuffer(part.indexBuffer);
    renderer.DestroyBuffer(part.vertexBuffer);
}

ChunkMesh::ChunkMesh(const ChunkMeshData& data)
{
    Upload(m_Opaque, data.opaque);
    Upload(m_Cross, data.cross);
    Upload(m_Transparent, data.transparent);
}

ChunkMesh::~ChunkMesh()
{
    Release(m_Transparent);
    Release(m_Cross);
    Release(m_Opaque);
}

namespace {

// Binds a part's vertex and index buffers and issues its indexed draw.
void DrawPart(VkCommandBuffer cmd, const GpuBuffer& vertexBuffer, const GpuBuffer& indexBuffer, uint32_t elementCount)
{
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, elementCount, 1, 0, 0, 0);
}

} // namespace

void ChunkMesh::DrawOpaque(VkCommandBuffer cmd) const
{
    if (m_Opaque.elementCount == 0) {
        return;
    }
    DrawPart(cmd, m_Opaque.vertexBuffer, m_Opaque.indexBuffer, m_Opaque.elementCount);
}

void ChunkMesh::DrawCross(VkCommandBuffer cmd) const
{
    if (m_Cross.elementCount == 0) {
        return;
    }
    DrawPart(cmd, m_Cross.vertexBuffer, m_Cross.indexBuffer, m_Cross.elementCount);
}

void ChunkMesh::DrawTransparent(VkCommandBuffer cmd) const
{
    if (m_Transparent.elementCount == 0) {
        return;
    }
    DrawPart(cmd, m_Transparent.vertexBuffer, m_Transparent.indexBuffer, m_Transparent.elementCount);
}

void ChunkMesh::AddFaceToData(
    const std::array<glm::vec3, 4>& positionList,
    const std::array<glm::vec2, 2>& uvCoordsList,
    const glm::vec3& normal, float waterDepth, const glm::vec3& tint,
    const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    const uint32_t offset = static_cast<uint32_t>(vertexBufferData.size() / k_VertexStride);

    const std::array<glm::vec2, 4> uvs = {
        glm::vec2(uvCoordsList[0].x, uvCoordsList[0].y),
        glm::vec2(uvCoordsList[1].x, uvCoordsList[0].y),
        glm::vec2(uvCoordsList[1].x, uvCoordsList[1].y),
        glm::vec2(uvCoordsList[0].x, uvCoordsList[1].y)
    };

    for (size_t i = 0; i < 4; i++) {
        vertexBufferData.push_back(positionList[i].x);
        vertexBufferData.push_back(positionList[i].y);
        vertexBufferData.push_back(positionList[i].z);
        vertexBufferData.push_back(uvs[i].x);
        vertexBufferData.push_back(uvs[i].y);
        vertexBufferData.push_back(normal.x);
        vertexBufferData.push_back(normal.y);
        vertexBufferData.push_back(normal.z);
        vertexBufferData.push_back(vertexLight[i]);
        vertexBufferData.push_back(waterDepth);
        vertexBufferData.push_back(tint.x);
        vertexBufferData.push_back(tint.y);
        vertexBufferData.push_back(tint.z);
        vertexBufferData.push_back(vertexBlockLight[i]);
    }

    // Split along the diagonal that avoids anisotropic shading on a face with
    // one occluded corner. Both triangles wind counter-clockwise when viewed
    // from outside (the quad order 0,1,2,3 is CCW there) so back-face culling
    // keeps the outward face.
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
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);
    glm::vec2 uvCoords = atlas.side;
    if (face == BlockFace::k_Top) {
        uvCoords = atlas.top;
    } else if (face == BlockFace::k_Bottom) {
        uvCoords = atlas.bottom;
    }

    const FaceQuad quad = FaceGeometryOf(position, face);
    std::array<glm::vec3, 4> positionList = {
        quad.origin, quad.origin + quad.dx, quad.origin + quad.dx + quad.dy, quad.origin + quad.dy
    };

    // Cactus pulls its four side faces inward by 1/16 (the top and bottom stay
    // full), so the cap and base overhang the stem like Minecraft's model.
    if (block == Block::k_Cactus && face != BlockFace::k_Top && face != BlockFace::k_Bottom) {
        const glm::vec3 inset = quad.normal * (1.0f / 16.0f);
        for (glm::vec3& vertex : positionList) {
            vertex -= inset;
        }
    }

    // Drop only the vertices on the block's top edge, so the top face lowers and
    // the side faces shrink to meet it while the bottom stays put.
    if (topInset > 0.0f) {
        for (glm::vec3& vertex : positionList) {
            if (vertex.y > position.y + 0.5f) {
                vertex.y -= topInset;
            }
        }
    }

    const std::array<glm::vec2, 2> uvCoordsList = { uvCoords, uvCoords + glm::vec2(BlockAtlas::k_Step) };

    AddFaceToData(positionList, uvCoordsList, quad.normal, waterDepth, tint, vertexLight, vertexBlockLight, vertexBufferData, elementBufferData);
}

void ChunkMesh::AddOverlayFace(
    const glm::vec3& position, BlockFace face,
    const glm::vec2& tile, const glm::vec3& tint,
    const std::array<float, 4>& vertexLight, const std::array<float, 4>& vertexBlockLight,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    // Coplanar with the dirt base: it shares the base's exact vertex positions,
    // so with GL_LEQUAL depth testing the overlay (emitted right after the base)
    // wins the equal-depth test and sits flush, with no z-fight and no lip.
    const FaceQuad quad = FaceGeometryOf(position, face);
    const std::array<glm::vec3, 4> positionList = {
        quad.origin, quad.origin + quad.dx, quad.origin + quad.dx + quad.dy, quad.origin + quad.dy
    };

    const std::array<glm::vec2, 2> uvCoordsList = { tile, tile + glm::vec2(BlockAtlas::k_Step) };

    AddFaceToData(positionList, uvCoordsList, quad.normal, 0.0f, tint, vertexLight, vertexBlockLight, vertexBufferData, elementBufferData);
}

void ChunkMesh::AddCrossToData(
    const glm::vec3& position, const glm::vec2& tile, const glm::vec3& tint, float light, float blockLight,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    // Flat lighting: every corner takes the cell's own sky and block light, and
    // the normal points up so the plant catches sun like the ground it stands on.
    // The cross pass disables back-face culling, so each plane shows from both sides.
    const std::array<float, 4> vertexLight = { light, light, light, light };
    const std::array<float, 4> vertexBlockLight = { blockLight, blockLight, blockLight, blockLight };
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);
    const std::array<glm::vec2, 2> uvCoordsList = { tile, tile + glm::vec2(BlockAtlas::k_Step) };

    // Two diagonal planes, each wound origin -> +dx -> +dx+dy -> +dy so the V
    // axis runs bottom-to-top (matching the cube faces, so the tile is upright).
    const std::array<glm::vec3, 4> plane1 = {
        position + glm::vec3(0, 0, 0), position + glm::vec3(1, 0, 1),
        position + glm::vec3(1, 1, 1), position + glm::vec3(0, 1, 0)
    };
    AddFaceToData(plane1, uvCoordsList, normal, 0.0f, tint, vertexLight, vertexBlockLight, vertexBufferData, elementBufferData);

    const std::array<glm::vec3, 4> plane2 = {
        position + glm::vec3(1, 0, 0), position + glm::vec3(0, 0, 1),
        position + glm::vec3(0, 1, 1), position + glm::vec3(1, 1, 0)
    };
    AddFaceToData(plane2, uvCoordsList, normal, 0.0f, tint, vertexLight, vertexBlockLight, vertexBufferData, elementBufferData);
}

} // namespace Krafter
