#include <algorithm>

#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/World.h"

namespace Krafter {

World::World()
{
    Biome::LoadBiomes();

    uint32_t hardware = std::thread::hardware_concurrency();
    uint32_t threadCount = hardware > 1 ? hardware - 1 : 1;
    m_Workers.reserve(threadCount);
    for (uint32_t i = 0; i < threadCount; i++) {
        m_Workers.emplace_back([this] { WorkerLoop(); });
    }
}

World::~World()
{
    {
        std::lock_guard<std::mutex> lock(m_JobMutex);
        m_Stop = true;
    }
    m_JobCv.notify_all();
    for (auto& worker : m_Workers) {
        worker.join();
    }
}

void World::Update()
{
    DrainResults();

    glm::vec3 cameraPosition = Renderer::GetCamera()->GetPosition();
    glm::ivec2 origin = glm::ivec2(cameraPosition.x, cameraPosition.z) / Chunk::k_Width;

    for (int32_t x = -m_RenderDistance - 1; x <= m_RenderDistance + 1; x++) {
        for (int32_t z = -m_RenderDistance - 1; z <= m_RenderDistance + 1; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance + 1)) {
                continue;
            }
            if (m_Chunks.count(position) || m_PendingTerrain.count(position)) {
                continue;
            }
            m_PendingTerrain.insert(position);
            DispatchJob([this, position] {
                auto chunk = std::make_shared<Chunk>(position);
                std::lock_guard<std::mutex> lock(m_TerrainResultMutex);
                m_TerrainResults.push_back({ position, std::move(chunk) });
            });
        }
    }

    constexpr glm::ivec2 dp[] = {
        glm::ivec2(-1, 0),
        glm::ivec2(1, 0),
        glm::ivec2(0, -1),
        glm::ivec2(0, 1)
    };

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
            if (m_PendingMesh.count(position) || !HasTerrainNeighbours(position)) {
                continue;
            }

            std::shared_ptr<Chunk> centerChunk = it->second.chunk;
            std::array<std::shared_ptr<Chunk>, 4> neighbourChunks;
            for (size_t i = 0; i < 4; i++) {
                neighbourChunks[i] = m_Chunks.find(position + dp[i])->second.chunk;
            }

            m_PendingMesh.insert(position);
            DispatchJob([this, position, centerChunk = std::move(centerChunk), neighbourChunks = std::move(neighbourChunks)] {
                std::array<const Chunk*, 4> neighbourPtrs = {
                    neighbourChunks[0].get(),
                    neighbourChunks[1].get(),
                    neighbourChunks[2].get(),
                    neighbourChunks[3].get()
                };
                ChunkMeshData data = ChunkMesh::Compute(*centerChunk, neighbourPtrs, position);
                std::lock_guard<std::mutex> lock(m_MeshResultMutex);
                m_MeshResults.push_back({ position, std::move(data) });
            });
        }
    }

    for (auto it = m_Chunks.begin(); it != m_Chunks.end();) {
        if (!IsInRadius(it->first, origin, m_RenderDistance + 1)) {
            it = m_Chunks.erase(it);
        } else {
            ++it;
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
    ImGui::InputInt("Max Mesh Uploads Per Frame", &m_MaxMeshUploadsPerFrame);
    ImGui::Text("Workers: %zu", m_Workers.size());
    ImGui::Text("Chunks: %zu", m_Chunks.size());
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
    if (it == m_Chunks.end() || !it->second.chunk) {
        return Block::k_Air;
    }
    return it->second.chunk->GetBlock(localPosition);
}

constexpr bool World::IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius)
{
    glm::ivec2 d = entity - origin;
    return d.x * d.x + d.y * d.y <= radius * radius;
}

void World::WorkerLoop()
{
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_JobMutex);
            m_JobCv.wait(lock, [this] { return m_Stop.load() || !m_Jobs.empty(); });
            if (m_Stop.load() && m_Jobs.empty()) {
                return;
            }
            job = std::move(m_Jobs.front());
            m_Jobs.pop_front();
        }
        job();
    }
}

void World::DispatchJob(std::function<void()> job)
{
    {
        std::lock_guard<std::mutex> lock(m_JobMutex);
        m_Jobs.push_back(std::move(job));
    }
    m_JobCv.notify_one();
}

void World::DrainResults()
{
    std::deque<TerrainResult> terrainResults;
    {
        std::lock_guard<std::mutex> lock(m_TerrainResultMutex);
        terrainResults.swap(m_TerrainResults);
    }
    for (auto& result : terrainResults) {
        m_PendingTerrain.erase(result.position);
        m_Chunks.try_emplace(result.position, std::move(result.chunk), ChunkState::k_TerrainReady);
    }

    size_t meshTake;
    std::deque<MeshResult> meshResults;
    {
        std::lock_guard<std::mutex> lock(m_MeshResultMutex);
        meshTake = std::min(static_cast<size_t>(std::max(m_MaxMeshUploadsPerFrame, 0)), m_MeshResults.size());
        for (size_t i = 0; i < meshTake; i++) {
            meshResults.push_back(std::move(m_MeshResults.front()));
            m_MeshResults.pop_front();
        }
    }
    for (auto& result : meshResults) {
        m_PendingMesh.erase(result.position);
        auto it = m_Chunks.find(result.position);
        if (it == m_Chunks.end()) {
            continue;
        }
        it->second.mesh = std::make_unique<ChunkMesh>(result.data);
        it->second.state = ChunkState::k_MeshReady;
    }
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
