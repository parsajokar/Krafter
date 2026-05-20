#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"
#include "Krafter/World/World.h"

namespace Krafter {

void World::Update()
{
    glm::vec3 cameraPosition = Renderer::GetCamera()->GetPosition();
    glm::ivec2 origin = glm::ivec2(cameraPosition.x, cameraPosition.z) / Chunk::k_Width;

    for (int32_t x = -m_RenderDistance - 1; x <= m_RenderDistance + 1; x++) {
        for (int32_t z = -m_RenderDistance - 1; z <= m_RenderDistance + 1; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance + 1)) {
                continue;
            }
            if (m_Chunks.count(position) || m_QueuedTerrain.count(position)) {
                continue;
            }
            m_TerrainQueue.push_back(position);
            m_QueuedTerrain.insert(position);
        }
    }

    for (int32_t x = -m_RenderDistance; x <= m_RenderDistance; x++) {
        for (int32_t z = -m_RenderDistance; z <= m_RenderDistance; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance)) {
                continue;
            }
            auto it = m_Chunks.find(position);
            if (it == m_Chunks.end() || it->second.state != ChunkState::k_TerrainReady) {
                continue;
            }
            if (m_QueuedMesh.count(position) || !HasTerrainNeighbours(position)) {
                continue;
            }
            m_MeshQueue.push_back(position);
            m_QueuedMesh.insert(position);
        }
    }

    for (const auto& [position, record] : m_Chunks) {
        if (IsInRadius(position, origin, m_RenderDistance + 1)) {
            continue;
        }
        if (m_QueuedUnload.count(position)) {
            continue;
        }
        m_UnloadQueue.push_back(position);
        m_QueuedUnload.insert(position);
    }

    if (Window::GetTime() > m_LastChunkUpdate + m_ChunkDelay) {
        m_LastChunkUpdate = Window::GetTime();

        if (!m_TerrainQueue.empty()) {
            glm::ivec2 target = m_TerrainQueue.front();
            m_TerrainQueue.pop_front();
            m_QueuedTerrain.erase(target);
            if (IsInRadius(target, origin, m_RenderDistance + 1)) {
                GenerateTerrain(target);
            }
        }

        if (!m_MeshQueue.empty()) {
            glm::ivec2 target = m_MeshQueue.front();
            m_MeshQueue.pop_front();
            m_QueuedMesh.erase(target);
            auto it = m_Chunks.find(target);
            if (it != m_Chunks.end()
                && it->second.state == ChunkState::k_TerrainReady
                && IsInRadius(target, origin, m_RenderDistance)
                && HasTerrainNeighbours(target)) {
                GenerateMesh(target);
            }
        }
    }

    while (!m_UnloadQueue.empty()) {
        glm::ivec2 target = m_UnloadQueue.front();
        m_UnloadQueue.pop_front();
        m_QueuedUnload.erase(target);
        if (!IsInRadius(target, origin, m_RenderDistance + 1)) {
            Unload(target);
        }
    }
}

void World::Render()
{
    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            Renderer::RenderChunkMesh(*record.mesh);
        }
    }
}

void World::RenderImGui()
{
    ImGui::InputInt("Render Distance", &m_RenderDistance);
    ImGui::InputFloat("Chunk Delay", &m_ChunkDelay);
}

Block World::GetBlock(const glm::ivec3& worldPosition) const
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return Block::k_Air;
    }

    auto floorDiv = [](int32_t a, int32_t b) {
        int32_t q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0))) {
            q--;
        }
        return q;
    };
    auto floorMod = [](int32_t a, int32_t b) {
        int32_t r = a % b;
        if (r != 0 && (r < 0) != (b < 0)) {
            r += b;
        }
        return r;
    };

    glm::ivec2 chunkPosition(
        floorDiv(worldPosition.x, Chunk::k_Width),
        floorDiv(worldPosition.z, Chunk::k_Width));
    glm::ivec3 localPosition(
        floorMod(worldPosition.x, Chunk::k_Width),
        worldPosition.y,
        floorMod(worldPosition.z, Chunk::k_Width));

    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end()) {
        return Block::k_Air;
    }
    return it->second.chunk.GetBlock(localPosition);
}

constexpr bool World::IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius)
{
    glm::ivec2 d = entity - origin;
    return d.x * d.x + d.y * d.y <= radius * radius;
}

void World::GenerateTerrain(const glm::ivec2& chunkPosition)
{
    m_Chunks.try_emplace(chunkPosition, chunkPosition);
}

void World::GenerateMesh(const glm::ivec2& chunkPosition)
{
    auto it = m_Chunks.find(chunkPosition);
    it->second.mesh = std::make_unique<ChunkMesh>(*this, chunkPosition);
    it->second.state = ChunkState::k_MeshReady;
}

void World::Unload(const glm::ivec2& chunkPosition)
{
    m_Chunks.erase(chunkPosition);
}

bool World::HasTerrainNeighbours(const glm::ivec2& chunkPosition) const
{
    constexpr glm::ivec2 dp[] = {
        glm::ivec2(-1, 0),
        glm::ivec2(1, 0),
        glm::ivec2(0, -1),
        glm::ivec2(0, 1)
    };
    for (size_t i = 0; i < 4; i++) {
        if (!m_Chunks.count(chunkPosition + dp[i])) {
            return false;
        }
    }
    return true;
}

} // namespace Krafter
