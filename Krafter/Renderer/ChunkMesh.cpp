#include "glad/gl.h"

#include "Krafter/Renderer/ChunkMesh.h"

namespace Krafter {

ChunkMesh::ChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition)
{
    std::vector<float> vertexBufferData;
    std::vector<uint32_t> elementBufferData;

    constexpr int32_t dx[] = { -1, 1, 0, 0, 0, 0 };
    constexpr int32_t dy[] = { 0, 0, -1, 1, 0, 0 };
    constexpr int32_t dz[] = { 0, 0, 0, 0, -1, 1 };
    constexpr BlockFace faces[] = {
        BlockFace::k_Front, BlockFace::k_Back,
        BlockFace::k_Bottom, BlockFace::k_Top,
        BlockFace::k_Left, BlockFace::k_Right
    };

    for (int32_t x = 0; x < Chunk::k_Width; x++) {
        for (int32_t y = 0; y < Chunk::k_Height; y++) {
            for (int32_t z = 0; z < Chunk::k_Width; z++) {
                if (chunkMap.at(chunkPosition).GetBlock(glm::ivec3(x, y, z)) == Block::k_Air) {
                    continue;
                }

                for (size_t k = 0; k < 6; k++) {
                    int32_t nx = x + dx[k];
                    int32_t ny = y + dy[k];
                    int32_t nz = z + dz[k];
                    BlockFace face = faces[k];

                    // TODO: Fix
                    if (nx < 0 || nx >= Chunk::k_Width || ny < 0 || ny >= Chunk::k_Height || nz < 0 || nz >= Chunk::k_Width) {
                        AddFaceToData(
                            glm::vec3(
                                chunkPosition.x * Chunk::k_Width + x,
                                y,
                                chunkPosition.y * Chunk::k_Width + z),
                            chunkMap.at(chunkPosition).GetBlock(glm::ivec3(x, y, z)), face,
                            vertexBufferData, elementBufferData);
                    } else if (chunkMap.at(chunkPosition).GetBlock(glm::ivec3(nx, ny, nz)) == Block::k_Air) {
                        AddFaceToData(
                            glm::vec3(
                                chunkMap.at(chunkPosition).GetPosition().x * Chunk::k_Width + x,
                                y,
                                chunkMap.at(chunkPosition).GetPosition().y * Chunk::k_Width + z),
                            chunkMap.at(chunkPosition).GetBlock(glm::ivec3(x, y, z)), face,
                            vertexBufferData, elementBufferData);
                    }
                }
            }
        }
    }

    m_ElementCount = elementBufferData.size();

    glCreateVertexArrays(1, &m_VertexArray);
    glCreateBuffers(1, &m_VertexBuffer);
    glCreateBuffers(1, &m_ElementBuffer);

    glNamedBufferData(m_VertexBuffer, vertexBufferData.size() * sizeof(float), vertexBufferData.data(), GL_STATIC_DRAW);
    glNamedBufferData(m_ElementBuffer, elementBufferData.size() * sizeof(uint32_t), elementBufferData.data(), GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_VertexArray, 0, m_VertexBuffer, 0, 5 * sizeof(float));
    glVertexArrayElementBuffer(m_VertexArray, m_ElementBuffer);

    glEnableVertexArrayAttrib(m_VertexArray, 0);
    glVertexArrayAttribBinding(m_VertexArray, 0, 0);
    glVertexArrayAttribFormat(m_VertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(m_VertexArray, 1);
    glVertexArrayAttribBinding(m_VertexArray, 1, 0);
    glVertexArrayAttribFormat(m_VertexArray, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
}

ChunkMesh::~ChunkMesh()
{
    glDeleteBuffers(1, &m_ElementBuffer);
    glDeleteBuffers(1, &m_VertexBuffer);
    glDeleteVertexArrays(1, &m_VertexArray);
}

void ChunkMesh::Bind() const
{
    glBindVertexArray(m_VertexArray);
}

void ChunkMesh::AddFaceToData(
    const std::array<glm::vec3, 4>& positionList,
    const std::array<glm::vec2, 2>& uvCoordsList,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    const size_t offset = vertexBufferData.size() / 5;

    vertexBufferData.push_back(positionList[0].x);
    vertexBufferData.push_back(positionList[0].y);
    vertexBufferData.push_back(positionList[0].z);
    vertexBufferData.push_back(uvCoordsList[0].x);
    vertexBufferData.push_back(uvCoordsList[0].y);

    vertexBufferData.push_back(positionList[1].x);
    vertexBufferData.push_back(positionList[1].y);
    vertexBufferData.push_back(positionList[1].z);
    vertexBufferData.push_back(uvCoordsList[1].x);
    vertexBufferData.push_back(uvCoordsList[0].y);

    vertexBufferData.push_back(positionList[2].x);
    vertexBufferData.push_back(positionList[2].y);
    vertexBufferData.push_back(positionList[2].z);
    vertexBufferData.push_back(uvCoordsList[1].x);
    vertexBufferData.push_back(uvCoordsList[1].y);

    vertexBufferData.push_back(positionList[3].x);
    vertexBufferData.push_back(positionList[3].y);
    vertexBufferData.push_back(positionList[3].z);
    vertexBufferData.push_back(uvCoordsList[0].x);
    vertexBufferData.push_back(uvCoordsList[1].y);

    elementBufferData.push_back(offset);
    elementBufferData.push_back(offset + 2);
    elementBufferData.push_back(offset + 1);

    elementBufferData.push_back(offset);
    elementBufferData.push_back(offset + 2);
    elementBufferData.push_back(offset + 3);
}

void ChunkMesh::AddFaceToData(
    const glm::vec3& position,
    const Block block, BlockFace face,
    std::vector<float>& vertexBufferData, std::vector<uint32_t>& elementBufferData)
{
    std::array<glm::vec3, 4> positionList;
    std::array<glm::vec2, 2> uvCoordsList;

    glm::vec3 origin;
    glm::vec3 dx;
    glm::vec3 dy;
    glm::vec2 uvCoords;

    const BlockAtlas& atlas = BlockAtlas::GetAtlasOf(block);

    switch (face) {
    case BlockFace::k_Front:
        origin = position;
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        break;

    case BlockFace::k_Back:
        origin = position + glm::vec3(1.0f, 0.0f, 1.0f);
        dx = glm::vec3(0.0f, 0.0f, -1.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        break;

    case BlockFace::k_Left:
        origin = position + glm::vec3(1.0f, 0.0f, 0.0f);
        dx = glm::vec3(-1.0f, 0.0f, 0.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        break;

    case BlockFace::k_Right:
        origin = position + glm::vec3(0.0f, 0.0f, 1.0f);
        dx = glm::vec3(1.0f, 0.0f, 0.0f);
        dy = glm::vec3(0.0f, 1.0f, 0.0f);
        uvCoords = atlas.side;
        break;

    case BlockFace::k_Bottom:
        origin = position + glm::vec3(1.0f, 0.0f, 0.0f);
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(-1.0f, 0.0f, 0.0f);
        uvCoords = atlas.bottom;
        break;

    default: // BlockFace::TOP
        origin = position + glm::vec3(0.0f, 1.0f, 0.0f);
        dx = glm::vec3(0.0f, 0.0f, 1.0f);
        dy = glm::vec3(1.0f, 0.0f, 0.0f);
        uvCoords = atlas.top;
        break;
    }

    positionList[0] = origin;
    positionList[1] = origin + dx;
    positionList[2] = origin + dx + dy;
    positionList[3] = origin + dy;

    uvCoordsList[0] = uvCoords;
    uvCoordsList[1] = uvCoords + glm::vec2(BlockAtlas::k_Step);

    AddFaceToData(positionList, uvCoordsList, vertexBufferData, elementBufferData);
}

} // namespace Krafter
