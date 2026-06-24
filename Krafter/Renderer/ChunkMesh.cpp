#include <array>
#include <cstdint>

#include "glad/gl.h"

#include "Krafter/Renderer/ChunkMesh.h"

namespace Krafter {

static int32_t FloorDiv(int32_t a, int32_t b)
{
    int32_t q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) {
        q--;
    }
    return q;
}

static int32_t FloorMod(int32_t a, int32_t b)
{
    int32_t r = a % b;
    if (r != 0 && (r < 0) != (b < 0)) {
        r += b;
    }
    return r;
}

// Ambient-occlusion brightness for the four corner levels (0 = most occluded).
static constexpr float k_AoFactor[] = { 0.5f, 0.7f, 0.85f, 1.0f };

// Floats per vertex: position(3), uv(2), normal(3), sky light(1), water depth(1).
static constexpr int32_t k_VertexStride = 10;

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

    // Only opaque blocks occlude and cast ambient occlusion; air and water let
    // their neighbours' faces through.
    auto isSolid = [&](const glm::ivec3& cell) -> bool {
        return IsOpaque(blockAt(cell));
    };

    // How much a block hides what is behind it: opaque (2) > water (1) > air (0).
    // A face is only drawn toward a strictly more transparent neighbour, so
    // opaque-vs-water and water-vs-air surfaces show while shared faces (e.g.
    // water-vs-water) are culled.
    auto opacityRank = [](Block block) -> int32_t {
        if (IsOpaque(block)) {
            return 2;
        }
        return block == Block::k_Water ? 1 : 0;
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

    // Smooth light + ambient occlusion for the face corner pointing toward
    // (su, sv), sampling the two side cells and the diagonal on the air side.
    auto cornerLight = [&](const glm::ivec3& airCell, const glm::ivec3& u, const glm::ivec3& v,
                           int32_t su, int32_t sv) -> float {
        const glm::ivec3 sideU = airCell + su * u;
        const glm::ivec3 sideV = airCell + sv * v;
        const glm::ivec3 corner = airCell + su * u + sv * v;

        const bool s1 = isSolid(sideU);
        const bool s2 = isSolid(sideV);
        const bool sc = isSolid(corner);

        const int32_t ao = (s1 && s2) ? 0 : 3 - (static_cast<int32_t>(s1) + static_cast<int32_t>(s2) + static_cast<int32_t>(sc));

        float sum = skyLightOf(airCell);
        int32_t count = 1;
        if (!s1) {
            sum += skyLightOf(sideU);
            count++;
        }
        if (!s2) {
            sum += skyLightOf(sideV);
            count++;
        }
        // Two solid edges seal off the diagonal, so its light can't reach this
        // vertex; sampling it anyway is what leaks light through missing corners.
        if (!sc && !(s1 && s2)) {
            sum += skyLightOf(corner);
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

                const bool transparent = self == Block::k_Water;
                ChunkMeshBuffer& buffer = transparent ? data.transparent : data.opaque;
                const int32_t selfRank = opacityRank(self);

                // Water that isn't submerged under more water sits below a full
                // block, so its surface reads as sunken. This holds even when a
                // solid block covers it: the inset leaves a visible gap.
                const bool surfaceWater = transparent && blockAt(glm::ivec3(x, y + 1, z)) != Block::k_Water;
                const float topInset = surfaceWater ? 4.0f / 16.0f : 0.0f;

                const float depth = transparent ? waterDepth(x, y, z) : 0.0f;

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
                    } else if (opacityRank(neighbor) >= selfRank) {
                        continue;
                    }

                    std::array<float, 4> vertexLight = {
                        cornerLight(airCell, faceU[k], faceV[k], -1, -1),
                        cornerLight(airCell, faceU[k], faceV[k], 1, -1),
                        cornerLight(airCell, faceU[k], faceV[k], 1, 1),
                        cornerLight(airCell, faceU[k], faceV[k], -1, 1)
                    };

                    glm::vec3 worldPos(
                        chunkPosition.x * Chunk::k_Width + x,
                        y,
                        chunkPosition.y * Chunk::k_Width + z);
                    AddFaceToData(worldPos, self, faces[k], topInset, depth, vertexLight, buffer.vertices, buffer.elements);
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

    glCreateVertexArrays(1, &part.vertexArray);
    glCreateBuffers(1, &part.vertexBuffer);
    glCreateBuffers(1, &part.elementBuffer);

    glNamedBufferData(part.vertexBuffer, buffer.vertices.size() * sizeof(float), buffer.vertices.data(), GL_STATIC_DRAW);
    glNamedBufferData(part.elementBuffer, buffer.elements.size() * sizeof(uint32_t), buffer.elements.data(), GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(part.vertexArray, 0, part.vertexBuffer, 0, k_VertexStride * sizeof(float));
    glVertexArrayElementBuffer(part.vertexArray, part.elementBuffer);

    glEnableVertexArrayAttrib(part.vertexArray, 0);
    glVertexArrayAttribBinding(part.vertexArray, 0, 0);
    glVertexArrayAttribFormat(part.vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(part.vertexArray, 1);
    glVertexArrayAttribBinding(part.vertexArray, 1, 0);
    glVertexArrayAttribFormat(part.vertexArray, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));

    glEnableVertexArrayAttrib(part.vertexArray, 2);
    glVertexArrayAttribBinding(part.vertexArray, 2, 0);
    glVertexArrayAttribFormat(part.vertexArray, 2, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float));

    glEnableVertexArrayAttrib(part.vertexArray, 3);
    glVertexArrayAttribBinding(part.vertexArray, 3, 0);
    glVertexArrayAttribFormat(part.vertexArray, 3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float));

    glEnableVertexArrayAttrib(part.vertexArray, 4);
    glVertexArrayAttribBinding(part.vertexArray, 4, 0);
    glVertexArrayAttribFormat(part.vertexArray, 4, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float));
}

void ChunkMesh::Release(Part& part)
{
    if (part.elementCount == 0) {
        return;
    }
    glDeleteBuffers(1, &part.elementBuffer);
    glDeleteBuffers(1, &part.vertexBuffer);
    glDeleteVertexArrays(1, &part.vertexArray);
}

ChunkMesh::ChunkMesh(const ChunkMeshData& data)
{
    Upload(m_Opaque, data.opaque);
    Upload(m_Transparent, data.transparent);
}

ChunkMesh::~ChunkMesh()
{
    Release(m_Transparent);
    Release(m_Opaque);
}

void ChunkMesh::BindOpaque() const
{
    glBindVertexArray(m_Opaque.vertexArray);
}

void ChunkMesh::BindTransparent() const
{
    glBindVertexArray(m_Transparent.vertexArray);
}

void ChunkMesh::AddFaceToData(
    const std::array<glm::vec3, 4>& positionList,
    const std::array<glm::vec2, 2>& uvCoordsList,
    const glm::vec3& normal, float waterDepth,
    const std::array<float, 4>& vertexLight,
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
    }

    // Split along the diagonal that avoids anisotropic shading on a face with
    // one occluded corner.
    if (vertexLight[0] + vertexLight[2] < vertexLight[1] + vertexLight[3]) {
        elementBufferData.push_back(offset + 1);
        elementBufferData.push_back(offset + 3);
        elementBufferData.push_back(offset + 0);

        elementBufferData.push_back(offset + 1);
        elementBufferData.push_back(offset + 3);
        elementBufferData.push_back(offset + 2);
    } else {
        elementBufferData.push_back(offset + 0);
        elementBufferData.push_back(offset + 2);
        elementBufferData.push_back(offset + 1);

        elementBufferData.push_back(offset + 0);
        elementBufferData.push_back(offset + 2);
        elementBufferData.push_back(offset + 3);
    }
}

void ChunkMesh::AddFaceToData(
    const glm::vec3& position,
    const Block block, BlockFace face, float topInset, float waterDepth,
    const std::array<float, 4>& vertexLight,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    std::array<glm::vec3, 4> positionList;
    std::array<glm::vec2, 2> uvCoordsList;

    glm::vec3 origin;
    glm::vec3 dx;
    glm::vec3 dy;
    glm::vec2 uvCoords;
    glm::vec3 normal;

    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);

    switch (face) {
    case BlockFace::k_Front:
        origin = position;
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        normal = glm::vec3(-1.0f, 0.0f, 0.0f);
        break;

    case BlockFace::k_Back:
        origin = position + glm::vec3(1.0f, 0.0f, 1.0f);
        dx = glm::vec3(0.0f, 0.0f, -1.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        normal = glm::vec3(1.0f, 0.0f, 0.0f);
        break;

    case BlockFace::k_Left:
        origin = position + glm::vec3(1.0f, 0.0f, 0.0f);
        dx = glm::vec3(-1.0f, 0.0f, 0.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        normal = glm::vec3(0.0f, 0.0f, -1.0f);
        break;

    case BlockFace::k_Right:
        origin = position + glm::vec3(0.0f, 0.0f, 1.0f);
        dx = glm::vec3(1.0f, 0.0f, 0.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        normal = glm::vec3(0.0f, 0.0f, 1.0f);
        break;

    case BlockFace::k_Bottom:
        origin = position + glm::vec3(1.0f, 0.0f, 0.0f);
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(-1.0f, 0.0f, 0.0f);
        uvCoords = atlas.bottom;
        normal = glm::vec3(0.0f, -1.0f, 0.0f);
        break;

    default:
        origin = position + glm::vec3(0.0f, 1.0f, 0.0f);
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(1.0f, 0.0f, 0.0f);
        uvCoords = atlas.top;
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    }

    positionList[0] = origin;
    positionList[1] = origin + dx;
    positionList[2] = origin + dx + dy;
    positionList[3] = origin + dy;

    // Drop only the vertices on the block's top edge, so the top face lowers and
    // the side faces shrink to meet it while the bottom stays put.
    if (topInset > 0.0f) {
        for (glm::vec3& vertex : positionList) {
            if (vertex.y > position.y + 0.5f) {
                vertex.y -= topInset;
            }
        }
    }

    uvCoordsList[0] = uvCoords;
    uvCoordsList[1] = uvCoords + glm::vec2(BlockAtlas::k_Step);

    AddFaceToData(positionList, uvCoordsList, normal, waterDepth, vertexLight, vertexBufferData, elementBufferData);
}

} // namespace Krafter
