#include <algorithm>
#include <limits>

#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Sky.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/Lighting.h"
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

void World::Update(const glm::vec3& cameraPosition)
{
    DrainResults();

    glm::ivec2 origin = glm::ivec2(cameraPosition.x, cameraPosition.z) / Chunk::k_Width;

    for (int32_t x = -m_RenderDistance - 2; x <= m_RenderDistance + 2; x++) {
        for (int32_t z = -m_RenderDistance - 2; z <= m_RenderDistance + 2; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance + 2)) {
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

    // Lighting stage: compute a chunk's sky light from its full 3x3 block
    // context and write it into the chunk's storage.
    for (int32_t x = -m_RenderDistance - 1; x <= m_RenderDistance + 1; x++) {
        for (int32_t z = -m_RenderDistance - 1; z <= m_RenderDistance + 1; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance + 1)) {
                continue;
            }
            auto it = m_Chunks.find(position);
            if (it == m_Chunks.end() || it->second.state != ChunkState::k_TerrainReady) {
                continue;
            }
            if (m_PendingLight.count(position) || !HasAllTerrainNeighbours(position)) {
                continue;
            }

            std::array<std::shared_ptr<Chunk>, 9> grid;
            for (int32_t dz = -1; dz <= 1; dz++) {
                for (int32_t dx = -1; dx <= 1; dx++) {
                    grid[(dz + 1) * 3 + (dx + 1)] = m_Chunks.find(position + glm::ivec2(dx, dz))->second.chunk;
                }
            }

            m_PendingLight.insert(position);
            DispatchJob([this, position, grid = std::move(grid)] {
                std::array<const Chunk*, 9> gridPtrs;
                for (size_t i = 0; i < 9; i++) {
                    gridPtrs[i] = grid[i].get();
                }
                ComputeSkyLight(*grid[4], gridPtrs);
                std::lock_guard<std::mutex> lock(m_LightResultMutex);
                m_LightResults.push_back(position);
            });
        }
    }

    // Meshing stage: only once the chunk and all 8 neighbours are lit, so
    // border smooth-lighting and AO sample settled, seam-free light.
    for (int32_t x = -m_RenderDistance; x <= m_RenderDistance; x++) {
        for (int32_t z = -m_RenderDistance; z <= m_RenderDistance; z++) {
            glm::ivec2 position = origin + glm::ivec2(x, z);
            if (!IsInRadius(position, origin, m_RenderDistance)) {
                continue;
            }
            auto it = m_Chunks.find(position);
            if (it == m_Chunks.end() || it->second.state != ChunkState::k_LightReady) {
                continue;
            }
            if (m_PendingMesh.count(position) || !HasAllLitNeighbours(position)) {
                continue;
            }

            std::array<std::shared_ptr<Chunk>, 9> grid;
            for (int32_t dz = -1; dz <= 1; dz++) {
                for (int32_t dx = -1; dx <= 1; dx++) {
                    grid[(dz + 1) * 3 + (dx + 1)] = m_Chunks.find(position + glm::ivec2(dx, dz))->second.chunk;
                }
            }

            m_PendingMesh.insert(position);
            DispatchJob([this, position, grid = std::move(grid)] {
                std::array<const Chunk*, 9> gridPtrs;
                for (size_t i = 0; i < 9; i++) {
                    gridPtrs[i] = grid[i].get();
                }
                ChunkMeshData data = ChunkMesh::Compute(gridPtrs, position);
                std::lock_guard<std::mutex> lock(m_MeshResultMutex);
                m_MeshResults.push_back({ position, std::move(data) });
            });
        }
    }

    for (auto it = m_Chunks.begin(); it != m_Chunks.end();) {
        if (!IsInRadius(it->first, origin, m_RenderDistance + 2)) {
            it = m_Chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void World::Render(Renderer& renderer, const glm::mat4& viewProjection, const Sky& sky)
{
    for (const auto& [position, record] : m_Chunks) {
        if (record.mesh) {
            renderer.RenderChunkMesh(*record.mesh, viewProjection, sky);
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

static glm::ivec2 ToChunkPosition(const glm::ivec3& worldPosition)
{
    return glm::ivec2(
        FloorDiv(worldPosition.x, Chunk::k_Width),
        FloorDiv(worldPosition.z, Chunk::k_Width));
}

static glm::ivec3 ToLocalPosition(const glm::ivec3& worldPosition)
{
    return glm::ivec3(
        FloorMod(worldPosition.x, Chunk::k_Width),
        worldPosition.y,
        FloorMod(worldPosition.z, Chunk::k_Width));
}

Block World::GetBlock(const glm::ivec3& worldPosition) const
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return Block::k_Air;
    }

    auto it = m_Chunks.find(ToChunkPosition(worldPosition));
    if (it == m_Chunks.end() || !it->second.chunk) {
        return Block::k_Air;
    }
    return it->second.chunk->GetBlock(ToLocalPosition(worldPosition));
}

void World::SetBlock(const glm::ivec3& worldPosition, Block block)
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return;
    }

    glm::ivec2 chunkPosition = ToChunkPosition(worldPosition);
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end() || !it->second.chunk) {
        return;
    }

    // Only edit settled terrain. A block on a chunk border changes its
    // neighbours' light and faces too, so the whole 3x3 must be meshed and have
    // no light/mesh job in flight; otherwise a worker could be reading a chunk
    // we are about to mutate or invalidate.
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            glm::ivec2 neighbour = chunkPosition + glm::ivec2(dx, dz);
            auto nit = m_Chunks.find(neighbour);
            if (nit == m_Chunks.end() || nit->second.state != ChunkState::k_MeshReady) {
                return;
            }
            if (m_PendingLight.count(neighbour) || m_PendingMesh.count(neighbour)) {
                return;
            }
        }
    }

    it->second.chunk->SetBlock(ToLocalPosition(worldPosition), block);

    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            InvalidateChunk(chunkPosition + glm::ivec2(dx, dz));
        }
    }
}

bool World::RaycastBlock(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, glm::ivec3& outHit, glm::ivec3& outBefore) const
{
    // Amanatides & Woo voxel traversal: walk cell by cell along the ray.
    glm::vec3 dir = glm::normalize(direction);
    glm::ivec3 block = glm::ivec3(glm::floor(origin));

    glm::ivec3 step;
    glm::vec3 tMax;
    glm::vec3 tDelta;
    for (int32_t i = 0; i < 3; i++) {
        if (dir[i] > 0.0f) {
            step[i] = 1;
            tMax[i] = (static_cast<float>(block[i] + 1) - origin[i]) / dir[i];
            tDelta[i] = 1.0f / dir[i];
        } else if (dir[i] < 0.0f) {
            step[i] = -1;
            tMax[i] = (origin[i] - static_cast<float>(block[i])) / -dir[i];
            tDelta[i] = 1.0f / -dir[i];
        } else {
            step[i] = 0;
            tMax[i] = std::numeric_limits<float>::infinity();
            tDelta[i] = std::numeric_limits<float>::infinity();
        }
    }

    float t = 0.0f;
    glm::ivec3 previous = block;
    while (t <= maxDistance) {
        if (GetBlock(block) != Block::k_Air) {
            outHit = block;
            outBefore = previous;
            return true;
        }

        previous = block;
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            block.x += step.x;
            t = tMax.x;
            tMax.x += tDelta.x;
        } else if (tMax.y < tMax.z) {
            block.y += step.y;
            t = tMax.y;
            tMax.y += tDelta.y;
        } else {
            block.z += step.z;
            t = tMax.z;
            tMax.z += tDelta.z;
        }
    }
    return false;
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

    std::deque<LightResult> lightResults;
    {
        std::lock_guard<std::mutex> lock(m_LightResultMutex);
        lightResults.swap(m_LightResults);
    }
    for (const auto& position : lightResults) {
        m_PendingLight.erase(position);
        auto it = m_Chunks.find(position);
        if (it != m_Chunks.end() && it->second.state == ChunkState::k_TerrainReady) {
            it->second.state = ChunkState::k_LightReady;
        }
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

void World::InvalidateChunk(const glm::ivec2& chunkPosition)
{
    auto it = m_Chunks.find(chunkPosition);
    if (it == m_Chunks.end()) {
        return;
    }

    // Drop back to the terrain stage so Update re-dispatches lighting and
    // meshing. The stale mesh keeps rendering until the new one is ready.
    it->second.state = ChunkState::k_TerrainReady;
    m_PendingLight.erase(chunkPosition);
    m_PendingMesh.erase(chunkPosition);
}

bool World::HasAllTerrainNeighbours(const glm::ivec2& chunkPosition) const
{
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dz == 0) {
                continue;
            }
            if (!m_Chunks.count(chunkPosition + glm::ivec2(dx, dz))) {
                return false;
            }
        }
    }
    return true;
}

bool World::HasAllLitNeighbours(const glm::ivec2& chunkPosition) const
{
    for (int32_t dz = -1; dz <= 1; dz++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dz == 0) {
                continue;
            }
            auto it = m_Chunks.find(chunkPosition + glm::ivec2(dx, dz));
            if (it == m_Chunks.end() || it->second.state == ChunkState::k_TerrainReady) {
                return false;
            }
        }
    }
    return true;
}

} // namespace Krafter
